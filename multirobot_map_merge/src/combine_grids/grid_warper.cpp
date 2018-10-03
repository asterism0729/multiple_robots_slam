/*********************************************************************
 *
 * Software License Agreement (BSD License)
 *
 *  Copyright (c) 2015-2016, Jiri Horner.
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above
 *     copyright notice, this list of conditions and the following
 *     disclaimer in the documentation and/or other materials provided
 *     with the distribution.
 *   * Neither the name of the Jiri Horner nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 *  FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 *  COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 *  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 *  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 *  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 *  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 *  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 *  ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 *
 *********************************************************************/

#include <combine_grids/grid_warper.h>

#include <opencv2/stitching/detail/warpers.hpp>

#include <ros/assert.h>

namespace combine_grids
{
namespace internal
{
cv::Rect GridWarper::warp(const cv::Mat& grid, const cv::Mat& transform,
                          cv::Mat& warped_grid)
{
  ROS_ASSERT(transform.type() == CV_64F);
  cv::Mat H;
  std::cout << "transform\n" << transform << '\n';
  invertAffineTransform(transform.rowRange(0, 2), H);//Hに逆アフィン
  cv::Rect roi = warpRoi(grid, H);//アフィンの逆でrectを移動
  // shift top left corner for warp affine (otherwise the image is cropped)
  std::cout << "before_H\n" << H << '\n';
  std::cout << "warp_roi\n" << roi << '\n';
  H.at<double>(0, 2) -= roi.tl().x;//warpAffineを回転だけにする
  H.at<double>(1, 2) -= roi.tl().y;//warpAffineを回転だけにする
  std::cout << "after_H\n" << H << '\n';
  std::cout << "warp_affine" << '\n';
  warpAffine(grid, warped_grid, H, roi.size(), cv::INTER_NEAREST,
             cv::BORDER_CONSTANT,
             cv::Scalar::all(255) /* this is -1 for signed char */);//grid:前景 warped:背景 H:前景の移動行列
  std::cout << "roi_size << " << roi.size() << '\n';
  std::cout << "warped_grid_size << " << warped_grid.size() << '\n';
  ROS_ASSERT(roi.size() == warped_grid.size());

  return roi;
}

cv::Rect GridWarper::warpRoi(const cv::Mat& grid, const cv::Mat& transform)
{
  cv::Ptr<cv::detail::PlaneWarper> warper =
      cv::makePtr<cv::detail::PlaneWarper>();
  cv::Mat H;
  transform.convertTo(H, CV_32F);

  std::cout << "H\n" << H << '\n';

  // separate rotation and translation for plane warper
  // 3D translation
  cv::Mat T = cv::Mat::zeros(3, 1, CV_32F);
  H.colRange(2, 3).rowRange(0, 2).copyTo(T.rowRange(0, 2));

  std::cout << "T\n" << T << '\n';


  std::cout << "T_size << " << T.size() << '\n';
  // 3D rotation
  cv::Mat R = cv::Mat::eye(3, 3, CV_32F);
  H.colRange(0, 2).copyTo(R.rowRange(0, 2).colRange(0, 2));

  std::cout << "R\n" << R << '\n';

  std::cout << "grid_size << " << grid.size() << '\n';

  return warper->warpRoi(grid.size(), cv::Mat::eye(3, 3, CV_32F), R, T);
}

}  // namespace internal

}  // namespace combine_grids
