/******************************************************************************

  This source file is part of the Avogadro project.

  Copyright 2012 Kitware, Inc.

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

******************************************************************************/

#include "iotests.h"

#include <gtest/gtest.h>

#include <avogadro/io/hdf5dataformat.h>

#include <cstdio>

using Avogadro::Io::Hdf5DataFormat;

TEST(Hdf5Test, openCloseReadOnly)
{
  Hdf5DataFormat hdf5;
  std::string testfile = std::string(AVOGADRO_DATA) + "/data/hdf5file.h5";
  ASSERT_TRUE(hdf5.openFile(testfile.c_str(), Hdf5DataFormat::ReadOnly))
      << "Failed to open " << testfile << " in read-only mode.";

  std::vector<std::string> refDatasets;
  refDatasets.resize(3);
  refDatasets[0] = "Data";
  refDatasets[1] = "Group1/Group2/Data";
  refDatasets[2] = "Test/MoleculeData/Matrix1";

  std::vector<std::string> datasets = hdf5.datasets();

  EXPECT_EQ(refDatasets, datasets) << "Unexpected list of datasets.";

  ASSERT_TRUE(hdf5.closeFile())
      << "Failed to close read-only file " << testfile << ".";
}

TEST(Hdf5Test, openCloseReadWriteAppend)
{
  Hdf5DataFormat hdf5;
  std::string testfile = std::string(AVOGADRO_DATA) + "/data/hdf5file.h5";
  ASSERT_TRUE(hdf5.openFile(testfile.c_str(), Hdf5DataFormat::ReadWriteAppend))
      << "Failed to open " << testfile << " in read-write (append) mode.";

  std::vector<std::string> refDatasets;
  refDatasets.resize(3);
  refDatasets[0] = "Data";
  refDatasets[1] = "Group1/Group2/Data";
  refDatasets[2] = "Test/MoleculeData/Matrix1";

  std::vector<std::string> datasets = hdf5.datasets();

  EXPECT_EQ(refDatasets, datasets) << "Unexpected list of datasets.";

  ASSERT_TRUE(hdf5.closeFile())
      << "Failed to close read-only file " << testfile << ".";
}

TEST(Hdf5Test, readWriteEigenMatrixXd)
{
  char tmpFileName [L_tmpnam];
  tmpnam(tmpFileName);

  Hdf5DataFormat hdf5;
  ASSERT_TRUE(hdf5.openFile(tmpFileName, Hdf5DataFormat::ReadWriteTruncate))
      << "Opening test file '" << tmpFileName << "' failed.";

  Eigen::MatrixXd mat (10, 10);
  for (int row = 0; row < 10; ++row) {
    for (int col = 0; col < 10; ++col) {
      mat(row, col) = row * col * col + row + col;
    }
  }

  EXPECT_TRUE(hdf5.writeDataset("/Group1/Group2/Data", mat))
      << "Writing Eigen::MatrixXd failed.";

  Eigen::MatrixXd matRead;
  EXPECT_TRUE(hdf5.readDataset("/Group1/Group2/Data", matRead))
      << "Reading Eigen::MatrixXd failed.";
  EXPECT_TRUE(mat.isApprox(matRead))
      << "Matrix read does not match matrix written.\nWritten:\n" << mat
      << "\nRead:\n" << matRead;

  ASSERT_TRUE(hdf5.closeFile())
      << "Closing test file '" << tmpFileName << "' failed.";

  remove(tmpFileName);
}

TEST(Hdf5Test, readWriteDoubleVector)
{
  char tmpFileName [L_tmpnam];
  tmpnam(tmpFileName);

  Hdf5DataFormat hdf5;
  ASSERT_TRUE(hdf5.openFile(tmpFileName, Hdf5DataFormat::ReadWriteTruncate))
      << "Opening test file '" << tmpFileName << "' failed.";

  std::vector<double> vec(100);
  size_t dims[2] = {10, 10};
  for (int i = 0; i < 100; ++i)
    vec[i] = i / 10.0 + i / 5.0;

  EXPECT_TRUE(hdf5.writeDataset("/Group1/Group2/Data", vec, 2, dims))
      << "Writing std::vector<double> failed.";

  std::vector<double> vecRead;
  std::vector<int> readDims = hdf5.readDataset("/Group1/Group2/Data", vecRead);
  EXPECT_EQ(readDims.size(), 2)
      << "Reading std::vector<double> failed: Invalid number of dimensions.";
  EXPECT_EQ(readDims.at(0), 10)
      << "Reading std::vector<double> failed: First dimension invalid.";
  EXPECT_EQ(readDims.at(1), 10)
      << "Reading std::vector<double> failed: Second dimension invalid.";
  for (int i = 0; i < 100; ++i) {
    EXPECT_EQ(vec[i], vecRead[i])
        << "std::vector<double> read/write mismatch at index " << i << ".";
  }

  ASSERT_TRUE(hdf5.closeFile())
      << "Closing test file '" << tmpFileName << "' failed.";

  remove(tmpFileName);
}

TEST(Hdf5Test, thresholds)
{
  Hdf5DataFormat hdf5;
  size_t threshold = 12;
  hdf5.setThreshold(threshold);
  EXPECT_EQ(hdf5.threshold(), threshold);

  EXPECT_FALSE(hdf5.exceedsThreshold(threshold - 1))
      << "Bad threshold check result for small data.";
  EXPECT_FALSE(hdf5.exceedsThreshold(threshold))
      << "Bad threshold check result for data at threshold limit.";
  EXPECT_TRUE(hdf5.exceedsThreshold(threshold + 1))
      << "Bad threshold check result for large data.";

  int numDoubles = threshold/sizeof(double);

  EXPECT_FALSE(hdf5.exceedsThreshold(Eigen::MatrixXd(1, numDoubles - 1)))
      << "Bad threshold check result for small data.";
  EXPECT_FALSE(hdf5.exceedsThreshold(Eigen::MatrixXd(1, numDoubles)))
      << "Bad threshold check result for data at threshold limit.";
  EXPECT_TRUE(hdf5.exceedsThreshold(Eigen::MatrixXd(1, numDoubles + 1)))
      << "Bad threshold check result for large data.";

  EXPECT_FALSE(hdf5.exceedsThreshold(std::vector<double>(numDoubles - 1)))
      << "Bad threshold check result for small data.";
  EXPECT_FALSE(hdf5.exceedsThreshold(std::vector<double>(numDoubles)))
      << "Bad threshold check result for data at threshold limit.";
  EXPECT_TRUE(hdf5.exceedsThreshold(std::vector<double>(numDoubles + 1)))
      << "Bad threshold check result for large data.";
}

TEST(Hdf5Test, datasetInteraction)
{
  char tmpFileName [L_tmpnam];
  tmpnam(tmpFileName);

  Hdf5DataFormat hdf5;
  ASSERT_TRUE(hdf5.openFile(tmpFileName, Hdf5DataFormat::ReadWriteTruncate))
      << "Opening test file '" << tmpFileName << "' failed.";

  Eigen::MatrixXd mat(1,1);
  mat(0, 0) = 0.0;

  std::vector<double> vec(27);
  int ndim_vec= 3;
  size_t dims_vec[3] = {3, 3, 3};

  EXPECT_TRUE(hdf5.writeDataset("/TLDData", vec, ndim_vec, dims_vec))
      << "Writing Eigen::MatrixXd failed.";
  EXPECT_TRUE(hdf5.writeDataset("/Group1/DeeperData", mat))
      << "Writing Eigen::MatrixXd failed.";
  EXPECT_TRUE(hdf5.writeDataset("/Group1/Group2/EvenDeeperData", mat))
      << "Writing Eigen::MatrixXd failed.";
  EXPECT_TRUE(hdf5.writeDataset("/Group1/DeeperDataSibling", mat))
      << "Writing Eigen::MatrixXd failed.";
  EXPECT_TRUE(hdf5.writeDataset("/Group1/Group2a/Grandchild", mat))
      << "Writing Eigen::MatrixXd failed.";
  EXPECT_TRUE(hdf5.writeDataset("/Group1/Group2a/Group3/Group4/Group5/Deeeep",
                                mat))
      << "Writing Eigen::MatrixXd failed.";
  EXPECT_TRUE(hdf5.writeDataset("/TLDataSibling", mat))
      << "Writing Eigen::MatrixXd failed.";


  std::vector<std::string> refDatasets;
  refDatasets.resize(7);
  refDatasets[0] = "Group1/DeeperData";
  refDatasets[1] = "Group1/DeeperDataSibling";
  refDatasets[2] = "Group1/Group2/EvenDeeperData";
  refDatasets[3] = "Group1/Group2a/Grandchild";
  refDatasets[4] = "Group1/Group2a/Group3/Group4/Group5/Deeeep";
  refDatasets[5] = "TLDData";
  refDatasets[6] = "TLDataSibling";
  EXPECT_EQ(refDatasets, hdf5.datasets()) << "List of dataset unexpected.";

  EXPECT_FALSE(hdf5.datasetExists("/IShouldNotExist"))
      << "Non-existing dataset reported as found.";

  std::vector<int> dim = hdf5.datasetDimensions("/Group1/DeeperData");
  EXPECT_EQ(dim.size(), 2) << "Wrong dimensionality returned.";
  EXPECT_EQ(dim[0], 1) << "Wrong dimensionality returned.";
  EXPECT_EQ(dim[1], 1) << "Wrong dimensionality returned.";

  dim = hdf5.datasetDimensions("/TLDData");
  EXPECT_EQ(dim.size(), ndim_vec) << "Wrong dimensionality returned.";
  for (int i = 0; i < ndim_vec; ++i) {
    EXPECT_EQ(dim[i], dims_vec[i]) << "Wrong dimensionality returned at " << i;
  }

  for (size_t i = 0; i < refDatasets.size(); ++i) {
    const std::string &str = refDatasets[i];
    EXPECT_TRUE(hdf5.datasetExists(str))
        << "Data set should exist, but isn't found: " << str;
    EXPECT_TRUE(hdf5.removeDataset(str))
        << "Error removing dataset " << str;
    EXPECT_FALSE(hdf5.datasetExists(str))
        << "Removed dataset still exists: " << str;
  }

  ASSERT_TRUE(hdf5.closeFile())
      << "Closing test file '" << tmpFileName << "' failed.";

  remove(tmpFileName);
}
