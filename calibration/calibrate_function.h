#pragma once
#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp> 
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/calib3d/calib3d.hpp>
#include <iostream>  
#include <fstream>
#include <map>
#include "../firmware/camera_param.h"

struct BoardMessage
{
	int rows;
	int cols;
	int width;
	int height;
};

class Calibrate_Function
{

public:

	Calibrate_Function();
	~Calibrate_Function();

	bool bilinearInterpolationFeaturePoints(std::vector<cv::Point2f> feature_points, std::vector<cv::Point3f>& point_3d, cv::Mat point_cloud);
	 
	double Bilinear_interpolation(double x, double y, cv::Mat& mapping);

	double computeLineError(std::vector<cv::Point2f> points, double max_err = 0.0);

	bool savePointsTxt(std::vector<cv::Point2f> points, std::string path);

	bool saveMatTxt(cv::Mat mat, std::string path);

	bool writeCalibTxt(cv::Mat camera_intrinsic, cv::Mat camera_distortion, cv::Mat projector_instrinsic, cv::Mat projector_distortion, cv::Mat s_r, cv::Mat s_t,std::string path);

	bool writeCalibXml(cv::Mat camera_intrinsic, cv::Mat camera_distortion, cv::Mat projector_instrinsic, cv::Mat projector_distortion, cv::Mat s_r, cv::Mat s_t);

	int readCalibParam(std::string path, struct CameraCalibParam& param);

	double correctExtrinsics(std::vector<std::vector<cv::Point2f>> camera_points_list,
		std::vector<std::vector<cv::Point2f>> dlp_points_list, std::string in_path, std::string out_path);

	double calibrateStereo(std::vector<std::vector<cv::Point2f>> camera_points_list, std::vector<std::vector<cv::Point2f>> dlp_points_list, std::string path);

	double calibrateProjector(std::vector<std::vector<cv::Point2f>> dlp_points_list, std::map<int, bool>& select_group);

	bool cameraPointsToDlp(std::vector<cv::Point2f> camera_points, cv::Mat unwrap_map_hor, cv::Mat unwrap_map_ver, int group_num,
		int dlp_width, int dlp_height, std::vector<cv::Point2f>& dlp_points);
	 
	bool fillThePhaseHole(cv::Mat& phase, bool is_hor);

	double calibrateCamera(std::vector<std::vector<cv::Point2f>> camera_points_list, std::map<int, bool>& select_group);

	cv::Mat inv_image(cv::Mat img);

	cv::Vec3f rotationMatrixToEulerAngles(cv::Mat& R); 

	bool findCircleBoardFeature(cv::Mat img, std::vector<cv::Point2f>& points);

	int testOverExposure(cv::Mat img, std::vector<cv::Point2f> points);

	cv::Size getBoardSize();

	std::vector<cv::Point3f> generateAsymmetricWorldFeature(struct BoardMessage board_message);

	std::vector<cv::Point3f> generateAsymmetricWorldFeature(float width, float height);

	std::vector<cv::Point3f> generateSymmetricWorldFeature(float width, float height);

	void setBoardMessage(int rows, int cols, int width, int height);

	void setBoardMessage(BoardMessage board_message);

	void setCalibrationBoard(int board_num);

private:

	BoardMessage board_message_;
	//cv::Size board_size_;
	 
	//float board_width_;
	//float board_height_;

	double dlp_width_;
	double dlp_height_;

	cv::Mat M_1_;
	cv::Mat M_2_;

	cv::Mat camera_intrinsic_;
	cv::Mat project_intrinsic_;
	cv::Mat camera_distortion_;
	cv::Mat projector_distortion_;
	cv::Mat rotation_matrix_;
	cv::Mat translation_matrix_;
};

