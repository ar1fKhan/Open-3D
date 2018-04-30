// ----------------------------------------------------------------------------
// -                        Open3D: www.open-3d.org                            -
// ----------------------------------------------------------------------------
// The MIT License (MIT)
//
// Copyright (c) 2018, Intel Visual Computing Lab
// Copyright (c) 2018, Open3D community
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
// IN THE SOFTWARE.
// ----------------------------------------------------------------------------

#pragma once

#include <vector>
#include <memory>
#include <Eigen/Core>

#include <Open3D/Core/Geometry/Geometry.h>
#include <Open3D/Core/Geometry/KDTreeSearchParam.h>
#include <Open3D/Core/Registration/Feature.h>

namespace flann {
template <typename T> class Matrix;
template <typename T> struct L2;
template <typename T> class Index;
}   // namespace flann

namespace open3d {

class KDTreeFlann
{
public:
    KDTreeFlann();
    KDTreeFlann(const Eigen::MatrixXd &data);
    KDTreeFlann(const Geometry &geometry);
    KDTreeFlann(const Feature &feature);
    ~KDTreeFlann();
    KDTreeFlann(const KDTreeFlann &) = delete;
    KDTreeFlann &operator=(const KDTreeFlann &) = delete;

public:
    bool SetMatrixData(const Eigen::MatrixXd &data);
    bool SetGeometry(const Geometry &geometry);
    bool SetFeature(const Feature &feature);

    template<typename T>
    int32_t Search(const T &query, const KDTreeSearchParam &param,
            std::vector<int32_t> &indices, std::vector<double> &distance2) const;

    template<typename T>
    int32_t SearchKNN(const T &query, int32_t knn, std::vector<int32_t> &indices,
            std::vector<double> &distance2) const;

    template<typename T>
    int32_t SearchRadius(const T &query, double radius, std::vector<int32_t> &indices,
            std::vector<double> &distance2) const;

    template<typename T>
    int32_t SearchHybrid(const T &query, double radius, int32_t max_nn,
            std::vector<int32_t> &indices, std::vector<double> &distance2) const;

private:
    bool SetRawData(const Eigen::Map<const Eigen::MatrixXd> &data);

protected:
    std::vector<double> data_;
    std::unique_ptr<flann::Matrix<double>> flann_dataset_;
    std::unique_ptr<flann::Index<flann::L2<double>>> flann_index_;
    size_t dimension_ = 0;
    size_t dataset_size_ = 0;
};

}   // namespace open3d
