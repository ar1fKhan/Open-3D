// ----------------------------------------------------------------------------
// -                        Open3D: www.open-3d.org                            -
// ----------------------------------------------------------------------------
// The MIT License (MIT)
//
// Copyright (c) 2018, Intel Labs
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

#include <Open3D/Core/Registration/FastGlobalRegistration.h>

#include <ctime>

#include <Open3D/Core/Geometry/PointCloud.h>
#include <Open3D/Core/Geometry/KDTreeFlann.h>
#include <Open3D/Core/Registration/Registration.h>
#include <Open3D/Core/Registration/Feature.h>
#include <Open3D/Core/Utility/Console.h>

namespace open3d {

namespace {

std::vector<Eigen::Vector3d> Means;
double GlobalScale;
double StartScale;
std::vector<std::shared_ptr<PointCloud>> pointcloud_;
std::vector<std::shared_ptr<Feature>> features_;
std::vector<std::pair<size_t, size_t>> corres_;
Eigen::Matrix4d TransOutput_;

void AdvancedMatching(const FastGlobalRegistrationOption& option)
{
    int32_t fi = 0;
    int32_t fj = 1;

    PrintDebug("Advanced matching : [%d - %d]\n", fi, fj);
    bool swapped = false;

    if (pointcloud_[fj]->points_.size() > pointcloud_[fi]->points_.size())
    {
        int32_t temp = fi;
        fi = fj;
        fj = temp;
        swapped = true;
    }

    size_t nPti = pointcloud_[fi]->points_.size();
    size_t nPtj = pointcloud_[fj]->points_.size();

    ///////////////////////////
    /// BUILD FLANNTREE
    ///////////////////////////

    // build FLANNTrees
    KDTreeFlann feature_tree_i(*features_[fi]);
    KDTreeFlann feature_tree_j(*features_[fj]);

    bool crosscheck = true;
    bool tuple = true;

    std::vector<int32_t> corres_K, corres_K2;
    std::vector<double> dis;
    std::vector<int32_t> ind;

    std::vector<std::pair<size_t, size_t>> corres;
    std::vector<std::pair<size_t, size_t>> corres_cross;
    std::vector<std::pair<size_t, size_t>> corres_ij;
    std::vector<std::pair<size_t, size_t>> corres_ji;

    ///////////////////////////
    /// INITIAL MATCHING
    ///////////////////////////

    std::vector<int32_t> i_to_j(nPti, -1);
    for (uint32_t j = 0; j < nPtj; j++)
    {
        feature_tree_i.SearchKNN(Eigen::VectorXd(features_[fj]->data_.col(j)),
                1, corres_K, dis);
        int32_t i = corres_K[0];
        if (i_to_j[i] == -1)
        {
            feature_tree_j.SearchKNN(
                    Eigen::VectorXd(features_[fi]->data_.col(i)),
                    1, corres_K, dis);
            int32_t ij = corres_K[0];
            i_to_j[i] = ij;
        }
        corres_ji.push_back(std::pair<size_t, size_t>(i, j));
    }

    for (uint32_t i = 0; i < nPti; i++)
    {
        if (i_to_j[i] != -1)
            corres_ij.push_back(std::pair<size_t, size_t>(i, i_to_j[i]));
    }


    size_t ncorres_ij = corres_ij.size();
    size_t ncorres_ji = corres_ji.size();

    // corres = corres_ij + corres_ji;
    for (size_t i = 0; i < ncorres_ij; ++i)
        corres.push_back(std::pair<size_t, size_t>(corres_ij[i].first, corres_ij[i].second));
    for (size_t j = 0; j < ncorres_ji; ++j)
        corres.push_back(std::pair<size_t, size_t>(corres_ji[j].first, corres_ji[j].second));

    PrintDebug("points are remained : %zu\n", corres.size());

    ///////////////////////////
    /// CROSS CHECK
    /// input : corres_ij, corres_ji
    /// output : corres
    ///////////////////////////
    if (crosscheck)
    {
        PrintDebug("\t[cross check] ");

        // build data structure for cross check
        corres.clear();
        corres_cross.clear();
        std::vector<std::vector<size_t>> Mi(nPti);
        std::vector<std::vector<size_t>> Mj(nPtj);

        size_t ci, cj;
        for (size_t i = 0; i < ncorres_ij; ++i)
        {
            ci = corres_ij[i].first;
            cj = corres_ij[i].second;
            Mi[ci].push_back(cj);
        }
        for (size_t j = 0; j < ncorres_ji; ++j)
        {
            ci = corres_ji[j].first;
            cj = corres_ji[j].second;
            Mj[cj].push_back(ci);
        }

        // cross check
        for (size_t i = 0; i < nPti; ++i)
        {
            for (size_t ii = 0; ii < Mi[i].size(); ++ii)
            {
                size_t j = Mi[i][ii];
                for (size_t jj = 0; jj < Mj[j].size(); ++jj)
                {
                    if (Mj[j][jj] == i)
                    {
                        corres.push_back(std::pair<size_t, size_t>(i, j));
                        corres_cross.push_back(std::pair<size_t, size_t>(i, j));
                    }
                }
            }
        }
        PrintDebug("points are remained : %zu\n", corres.size());
    }

    ///////////////////////////
    /// TUPLE CONSTRAINT
    /// input : corres
    /// output : corres
    ///////////////////////////
    if (tuple)
    {
        std::srand(static_cast<unsigned long>(std::time(0)));

        PrintDebug("\t[tuple constraint] ");
        int32_t rand0, rand1, rand2;
        size_t idi0, idi1, idi2;
        size_t idj0, idj1, idj2;
        double scale = option.tuple_scale_;
        size_t ncorr = corres.size();
        size_t number_of_trial = ncorr * 100;
        std::vector<std::pair<size_t, size_t>> corres_tuple;

        size_t cnt = 0;
        size_t i;
        for (i = 0; i < number_of_trial; i++)
        {
            rand0 = rand() % ncorr;
            rand1 = rand() % ncorr;
            rand2 = rand() % ncorr;

            idi0 = corres[rand0].first;
            idj0 = corres[rand0].second;
            idi1 = corres[rand1].first;
            idj1 = corres[rand1].second;
            idi2 = corres[rand2].first;
            idj2 = corres[rand2].second;

            // collect 3 points from i-th fragment
            Eigen::Vector3d pti0 = pointcloud_[fi]->points_[idi0];
            Eigen::Vector3d pti1 = pointcloud_[fi]->points_[idi1];
            Eigen::Vector3d pti2 = pointcloud_[fi]->points_[idi2];

            double li0 = (pti0 - pti1).norm();
            double li1 = (pti1 - pti2).norm();
            double li2 = (pti2 - pti0).norm();

            // collect 3 points from j-th fragment
            Eigen::Vector3d ptj0 = pointcloud_[fj]->points_[idj0];
            Eigen::Vector3d ptj1 = pointcloud_[fj]->points_[idj1];
            Eigen::Vector3d ptj2 = pointcloud_[fj]->points_[idj2];

            double lj0 = (ptj0 - ptj1).norm();
            double lj1 = (ptj1 - ptj2).norm();
            double lj2 = (ptj2 - ptj0).norm();

            if ((li0 * scale < lj0) && (lj0 < li0 / scale) &&
                (li1 * scale < lj1) && (lj1 < li1 / scale) &&
                (li2 * scale < lj2) && (lj2 < li2 / scale))
            {
                corres_tuple.push_back(std::pair<size_t, size_t>(idi0, idj0));
                corres_tuple.push_back(std::pair<size_t, size_t>(idi1, idj1));
                corres_tuple.push_back(std::pair<size_t, size_t>(idi2, idj2));
                cnt++;
            }

            if (cnt >= option.maximum_tuple_count_)
                break;
        }

        PrintDebug("%zu tuples (%zu trial, %zu actual).\n", cnt, number_of_trial, i);
        corres.clear();

        for (size_t i = 0; i < corres_tuple.size(); ++i)
            corres.push_back(std::pair<size_t, size_t>(corres_tuple[i].first, corres_tuple[i].second));
    }

    if (swapped)
    {
        std::vector<std::pair<size_t, size_t>> temp;
        for (size_t i = 0; i < corres.size(); i++)
            temp.push_back(std::pair<size_t, size_t>(corres[i].second, corres[i].first));
        corres.clear();
        corres = temp;
    }

    PrintDebug("\t[final] matches %zu.\n", corres.size());
    corres_ = corres;
}


// Normalize scale of points.
// X' = (X-\mu)/scale
void NormalizePoints(const FastGlobalRegistrationOption& option)
{
    uint8_t num = 2;
    double scale = 0;

    Means.clear();

    for (uint8_t i = 0; i < num; ++i)
    {
        double max_scale = 0.0;

        // compute mean
        Eigen::Vector3d mean;
        mean.setZero();

        size_t npti = pointcloud_[i]->points_.size();
        for (size_t ii = 0; ii < npti; ++ii)
        {
            mean = mean + pointcloud_[i]->points_[ii];
        }
        mean = mean / npti;
        Means.push_back(mean);

        PrintDebug("normalize points :: mean = [%f %f %f]\n", mean(0), mean(1), mean(2));

        for (size_t ii = 0; ii < npti; ++ii)
        {
            pointcloud_[i]->points_[ii] -= mean;
        }

        // compute scale
        for (size_t ii = 0; ii < npti; ++ii)
        {
            Eigen::Vector3d p(pointcloud_[i]->points_[ii]);
            double temp = p.norm(); // because we extract mean in the previous stage.
            if (temp > max_scale)
                max_scale = temp;
        }

        if (max_scale > scale)
            scale = max_scale;
    }

    // mean of the scale variation
    if (option.use_absolute_scale_) {
        GlobalScale = 1.0f;
        StartScale = scale;
    } else {
        GlobalScale = scale; // second choice: we keep the maximum scale.
        StartScale = 1.0f;
    }
    PrintDebug("normalize points :: global scale : %f\n", GlobalScale);

    for (uint8_t i = 0; i < num; ++i)
    {
        size_t npti = pointcloud_[i]->points_.size();
        for (size_t ii = 0; ii < npti; ++ii)
        {
            pointcloud_[i]->points_[ii] /= GlobalScale;
        }
    }
}

double OptimizePairwise(const FastGlobalRegistrationOption& option)
{
    PrintDebug("Pairwise rigid pose optimization\n");

    double par;
    size_t numIter = option.iteration_number_;
    TransOutput_ = Eigen::Matrix4d::Identity();

    par = StartScale;

    int32_t i = 0;
    int32_t j = 1;

    // make another copy of pointcloud_[j].
    std::vector<Eigen::Vector3d> pcj_copy;
    size_t npcj = pointcloud_[j]->points_.size();
    pcj_copy.resize(npcj);
    for (size_t cnt = 0; cnt < npcj; cnt++)
        pcj_copy[cnt] = pointcloud_[j]->points_[cnt];

    if (corres_.size() < 10)
        return -1;

    std::vector<double> s(corres_.size(), 1.0);

    Eigen::Matrix4d trans;
    trans.setIdentity();

    for (size_t itr = 0; itr < numIter; itr++) {

        // graduated non-convexity.
        if (option.decrease_mu_)
        {
            if (itr % 4 == 0 && par > option.maximum_correspondence_distance_) {
                par /= option.division_factor_;
            }
        }

        const int32_t nvariable = 6;    // 3 for rotation and 3 for translation
        Eigen::MatrixXd JTJ(nvariable, nvariable);
        Eigen::MatrixXd JTr(nvariable, 1);
        Eigen::MatrixXd J(nvariable, 1);
        JTJ.setZero();
        JTr.setZero();

        double r;
        double r2 = 0.0;

        for (size_t c = 0; c < corres_.size(); c++) {
            size_t ii = corres_[c].first;
            size_t jj = corres_[c].second;
            Eigen::Vector3d p, q;
            p = pointcloud_[i]->points_[ii];
            q = pcj_copy[jj];
            Eigen::Vector3d rpq = p - q;

            size_t c2 = c;

            double temp = par / (rpq.dot(rpq) + par);
            s[c2] = temp * temp;

            J.setZero();
            J(1) = -q(2);
            J(2) = q(1);
            J(3) = -1;
            r = rpq(0);
            JTJ += J * J.transpose() * s[c2];
            JTr += J * r * s[c2];
            r2 += r * r * s[c2];

            J.setZero();
            J(2) = -q(0);
            J(0) = q(2);
            J(4) = -1;
            r = rpq(1);
            JTJ += J * J.transpose() * s[c2];
            JTr += J * r * s[c2];
            r2 += r * r * s[c2];

            J.setZero();
            J(0) = -q(1);
            J(1) = q(0);
            J(5) = -1;
            r = rpq(2);
            JTJ += J * J.transpose() * s[c2];
            JTr += J * r * s[c2];
            r2 += r * r * s[c2];

            r2 += (par * (1.0 - sqrt(s[c2])) * (1.0 - sqrt(s[c2])));
        }

        Eigen::MatrixXd result(nvariable, 1);
        result = -JTJ.llt().solve(JTr);

        Eigen::Affine3d aff_mat;
        aff_mat.linear() = (Eigen::Matrix3d) Eigen::AngleAxisd(result(2), Eigen::Vector3d::UnitZ())
            * Eigen::AngleAxisd(result(1), Eigen::Vector3d::UnitY())
            * Eigen::AngleAxisd(result(0), Eigen::Vector3d::UnitX());
        aff_mat.translation() = Eigen::Vector3d(result(3), result(4), result(5));

        Eigen::Matrix4d delta = aff_mat.matrix().cast<double>();

        trans = delta * trans;

        // transform point clouds
        Eigen::Matrix3d R = delta.block<3, 3>(0, 0);
        Eigen::Vector3d t = delta.block<3, 1>(0, 3);
        for (size_t cnt = 0; cnt < npcj; cnt++)
            pcj_copy[cnt] = R * pcj_copy[cnt] + t;

    }

    TransOutput_ = trans * TransOutput_;
    return par;
}

// Below line indicates how the transformation matrix aligns two point clouds
// e.g. T * pointcloud_[1] is aligned with pointcloud_[0].
// '2' indicates that there are two point cloud fragments.
Eigen::Matrix4d GetTrans()
{
    Eigen::Matrix3d R;
    Eigen::Vector3d t;
    R = TransOutput_.block<3, 3>(0, 0);
    t = TransOutput_.block<3, 1>(0, 3);

    Eigen::Matrix4d transtemp;
    transtemp.fill(0.0f);

    transtemp.block<3, 3>(0, 0) = R;
    transtemp.block<3, 1>(0, 3) = -R*Means[1] + t*GlobalScale + Means[0];
    transtemp(3, 3) = 1;

    return transtemp;
}

}    // unnamed namespace


RegistrationResult FastGlobalRegistration(
        const PointCloud &source, const PointCloud &target,
        const Feature &source_feature, const Feature &target_feature,
        const FastGlobalRegistrationOption &option/* =
        FastGlobalRegistrationOption()*/)
{
    std::shared_ptr<PointCloud> source_copy = std::make_shared<PointCloud>();
    std::shared_ptr<PointCloud> target_copy = std::make_shared<PointCloud>();
    *source_copy = source;
    *target_copy = target;
    pointcloud_.push_back(source_copy);
    pointcloud_.push_back(target_copy);

    std::shared_ptr<Feature> source_feature_copy = std::make_shared<Feature>();
    std::shared_ptr<Feature> target_feature_copy = std::make_shared<Feature>();
    *source_feature_copy = source_feature;
    *target_feature_copy = target_feature;
    features_.push_back(source_feature_copy);
    features_.push_back(target_feature_copy);

    NormalizePoints(option);
    AdvancedMatching(option);
    OptimizePairwise(option);

    // as the original code T * pointcloud_[1] is aligned with pointcloud_[0].
    // matrix inverse is applied here.
    return RegistrationResult(GetTrans().inverse());
}

}  // namespace open3d
