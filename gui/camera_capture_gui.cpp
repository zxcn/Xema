#include "camera_capture_gui.h"
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>
#include "file_io_function.h"
#include <QDebug>
#include <iostream>
#include <QMouseEvent>
#include <QtWidgets/qfiledialog.h>
#include <qheaderview.h>
#include "../calibration/calibrate_function.h"
#include "PrecisionTest.h"
 
 

CameraCaptureGui::CameraCaptureGui(QWidget *parent)
	: QWidget(parent)
{
	ui.setupUi(this);

	connected_flag_ = false; 

	ui.tableWidget_more_exposure->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
	ui.tableWidget_more_exposure->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Stretch);
	ui.tableWidget_more_exposure->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
	ui.tableWidget_more_exposure->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
	ui.tableWidget_more_exposure->setEditTriggers(QAbstractItemView::NoEditTriggers); //设置不可编辑
	ui.tableWidget_more_exposure->setFrameShape(QFrame::Box);

  
 
	config_system_param_machine_.loadProcessingSettingsFile("../camera_config.json");
	config_system_param_machine_.getFirmwareConfigData(system_config_param_);
	config_system_param_machine_.getGuiConfigData(processing_gui_settings_data_);

 

	radio_button_flag_ = SELECT_BRIGHTNESS_FLAG_;
	showImage();

	setUiData(); 
	initializeFunction();
	undateSystemConfigUiData();

	last_path_ = "../TestData";
	sys_path_ = "../TestData";
	QDir dir(last_path_);
	QString path = dir.absolutePath();

	if (!dir.exists(path))
	{
		bool res = dir.mkpath(path); 
	}

	//ui.tableWidget_more_exposure->hide();
	//ui.spinBox_exposure_num->hide();
	//ui.label_exposure_num->hide();
	ui.label_confidence->hide();
	ui.spinBox_confidence->hide();
	ui.pushButton_capture_many_frame->hide();
	ui.spinBox_multiframe->hide();

	//ui.radioButton_depth_grey->hide();

 
}

CameraCaptureGui::~CameraCaptureGui()
{
}


bool CameraCaptureGui::initializeFunction()
{
	/********************************************************************************************************************/
	  

	/*******************************************************************************************************************/

	connect(ui.spinBox_exposure_num, SIGNAL(valueChanged(int)), this, SLOT(do_spin_exposure_num_changed(int)));
	connect(ui.spinBox_min_z, SIGNAL(valueChanged(int)), this, SLOT(do_spin_min_z_changed(int)));
	connect(ui.spinBox_max_z, SIGNAL(valueChanged(int)), this, SLOT(do_spin_max_z_changed(int)));

	connect(ui.spinBox_led, SIGNAL(valueChanged(int)), this, SLOT(do_spin_led_current_changed(int)));

	connect(ui.radioButton_brightness, SIGNAL(toggled(bool)), this, SLOT(do_QRadioButton_toggled_brightness(bool)));
	connect(ui.radioButton_depth_color, SIGNAL(toggled(bool)), this, SLOT(do_QRadioButton_toggled_color_depth(bool)));
	connect(ui.radioButton_depth_grey, SIGNAL(toggled(bool)), this, SLOT(do_QRadioButton_toggled_gray_depth(bool)));


	connect(ui.checkBox_hdr, SIGNAL(toggled(bool)), this, SLOT(do_checkBox_toggled_hdr(bool)));

	connect(ui.pushButton_connect, SIGNAL(clicked()), this, SLOT(do_pushButton_connect()));
	connect(ui.pushButton_capture_one_frame, SIGNAL(clicked()), this, SLOT(do_pushButton_capture_one_frame()));
	connect(ui.pushButton_capture_continuous, SIGNAL(clicked()), this, SLOT(do_pushButton_capture_continuous()));
	connect(ui.pushButton_capture_many_frame, SIGNAL(clicked()), this, SLOT(do_pushButton_capture_many_frame()));


	connect(ui.pushButton_save_as, SIGNAL(clicked()), this, SLOT(do_pushButton_save_as()));
	connect(ui.pushButton_test_accuracy, SIGNAL(clicked()), this, SLOT(do_pushButton_test_accuracy()));
	connect(ui.pushButton_calibrate_external_param, SIGNAL(clicked()), this, SLOT(do_pushButton_calibrate_external_param()));

	connect(&capture_timer_, SIGNAL(timeout()), this, SLOT(do_timeout_capture_slot()));
	capture_timer_.setInterval(100);
	start_timer_flag_ = false;


	connect(this, SIGNAL(send_images_update()), this, SLOT(do_undate_show_slot()));
 

	min_depth_value_ = 300;
	max_depth_value_ = 1200;

	capture_show_flag_ = false;
	capturing_flag_ = false;
	camera_setting_flag_ = false;
	/**********************************************************************************************************************/




	return true;
}

 

void CameraCaptureGui::addLogMessage(QString str)
{

	QString StrCurrentTime =  QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz");

	QString log = StrCurrentTime + " " + str;

	ui.textBrowser_log->append(log);

	ui.textBrowser_log->repaint();
}
 

bool CameraCaptureGui::saveOneFrameData(QString path_name)
{
	if (path_name.isEmpty())
	{
		return false;
	}

	QDir dir(path_name);
	path_name = dir.absolutePath();

	 
	QString brightness_str = path_name + "_bright.bmp";
	cv::imwrite(brightness_str.toLocal8Bit().toStdString(), brightness_map_); 
	 
	QString depth_str = path_name + "_depth_map.tiff";
	cv::imwrite(depth_str.toLocal8Bit().toStdString(), depth_map_);

	QString height_str = path_name + "_height_map.tiff";
	cv::imwrite(height_str.toLocal8Bit().toStdString(), height_map_);

	QString points_str = path_name + ".ply"; 
	cv::Mat points_map(brightness_map_.size(), CV_32FC3, cv::Scalar(0., 0., 0.));

	depthTransformPointcloud((float*)depth_map_.data, (float*)points_map.data);


	FileIoFunction file_io_machine; 
	file_io_machine.SaveBinPointsToPly(points_map, points_str, brightness_map_);

	return true;
}


bool CameraCaptureGui::loadSettingData(QString path)
{
	bool ret =config_system_param_machine_.loadProcessingSettingsFile(path);
	if (!ret)
	{
		return false;
	}
	
	config_system_param_machine_.getFirmwareConfigData(system_config_param_);
	config_system_param_machine_.getGuiConfigData(processing_gui_settings_data_);
	   
	setUiData();
	undateSystemConfigUiData();

	return true;
}

bool CameraCaptureGui::saveSettingData(QString path)
{

	//bool ret = processing_settings_data_.saveToSettings(path);
	
	config_system_param_machine_.setFirmwareConfigData(system_config_param_);
	config_system_param_machine_.setGuiConfigData(processing_gui_settings_data_);

	bool ok = config_system_param_machine_.saveProcessingSettingsFile(path);
	
	return ok;
}


bool CameraCaptureGui::renderBrightnessImage(cv::Mat brightness)
{
	
	cv::Mat color_map(brightness.size(), CV_8UC3, cv::Scalar(0, 0, 0));

	int nr = color_map.rows;
	int nc = color_map.cols; 

	for (int r = 0; r < nr; r++)
	{
		uchar* ptr_b = brightness.ptr<uchar>(r);
		cv::Vec3b* ptr_cb = color_map.ptr<cv::Vec3b>(r);
		for (int c = 0; c < nc; c++)
		{
			if (ptr_b[c] == 255)
			{
				ptr_cb[c][0] = 255;
				ptr_cb[c][1] = 0;
				ptr_cb[c][2] = 0;
			}
			else
			{
				ptr_cb[c][0] = ptr_b[c];
				ptr_cb[c][1] = ptr_b[c];
				ptr_cb[c][2] = ptr_b[c];
			}
		}

	}

	render_image_brightness_ = color_map.clone();
	return true;
}


bool CameraCaptureGui::renderHeightImage(cv::Mat height)
{
	if (height.empty())
	{
		return false;
	}

	int low_z = processing_gui_settings_data_.Instance().low_z_value;
	int high_z = processing_gui_settings_data_.Instance().high_z_value;

	FileIoFunction io_machine; 

	io_machine.depthToColor(height, render_image_color_height_, render_image_gray_depth_, low_z, high_z);

	return true;

}

bool CameraCaptureGui::renderDepthImage(cv::Mat depth)
{
	if (depth.empty())
	{
		return false;
	}


	FileIoFunction io_machine;

	//int low_z = processing_settings_data_.Instance().low_z_value;
	//int high_z = processing_settings_data_.Instance().high_z_value;

	io_machine.depthToColor(depth, render_image_color_depth_, render_image_gray_depth_, min_depth_value_, max_depth_value_);

	return true;

}

bool CameraCaptureGui::setShowImages(cv::Mat brightness, cv::Mat depth)
{
	cv::Mat img_color;
	cv::Mat gray_map;
	FileIoFunction io_machine;

	int low_z = processing_gui_settings_data_.Instance().low_z_value;
	int high_z = processing_gui_settings_data_.Instance().high_z_value;

	io_machine.depthToColor(depth, img_color, gray_map, low_z, high_z);


	return true;
}


void CameraCaptureGui::setGuiSettingData(GuiConfigDataStruct& settings_data_)
{
	processing_gui_settings_data_ = settings_data_;
}



void CameraCaptureGui::undateSystemConfigUiData()
{
	ui.spinBox_led->setValue(system_config_param_.led_current);

	ui.spinBox_exposure_num->setValue(system_config_param_.exposure_num);

}

void CameraCaptureGui::setUiData()
{
	ui.spinBox_min_z->setValue(processing_gui_settings_data_.Instance().low_z_value);
	ui.spinBox_max_z->setValue(processing_gui_settings_data_.Instance().high_z_value);
	ui.lineEdit_ip->setText(processing_gui_settings_data_.Instance().ip);

	//ui.spinBox_exposure_num->setDisabled(true);
	//ui.spinBox_led->setDisabled(true);
}


double CameraCaptureGui::get_exposure_item_value(int row)
{
	if (row > ui.tableWidget_more_exposure->rowCount() - 1)
		return -1;

	return exposure_time_list_[row]->value();
 
}


bool CameraCaptureGui::remove_exposure_item(int row)
{

	int item_count = ui.tableWidget_more_exposure->rowCount();

	if (row > item_count - 1)
		return false;

	ui.tableWidget_more_exposure->removeRow(row); 

	return true;
}

 

void CameraCaptureGui::add_exposure_item(int row, int col, int val)
{
  

	int item_count = ui.tableWidget_more_exposure->rowCount();  
	
	if (row >= item_count)
	{
		ui.tableWidget_more_exposure->setRowCount(item_count + 1); 
	}
 
	QSpinBox* upperSpinBoxItem = new QSpinBox();
	upperSpinBoxItem->setRange(0, 1023);//设置数值显示范围
	upperSpinBoxItem->setValue(val);
	upperSpinBoxItem->setButtonSymbols(QAbstractSpinBox::NoButtons);
	upperSpinBoxItem->setAlignment(Qt::AlignHCenter);


	ui.tableWidget_more_exposure->setItem(row, 0 + 2* col, new QTableWidgetItem(QString::number(2*row + col + 1)));
	ui.tableWidget_more_exposure->setCellWidget(row, 1 + 2 * col, upperSpinBoxItem);//i为所在行，j+2为所在列
	 
	ui.tableWidget_more_exposure->setColumnWidth(2 * col,10);
	ui.tableWidget_more_exposure->item(row, +2 * col)->setTextAlignment(Qt::AlignHCenter | Qt::AlignCenter);
	ui.tableWidget_more_exposure->item(row, +2 * col)->setSelected(false);

	exposure_time_list_.push_back(upperSpinBoxItem);

}

void CameraCaptureGui::do_spin_min_z_changed(int val)
{
	processing_gui_settings_data_.Instance().low_z_value = val;

	renderHeightImage(height_map_);
	showImage();
	 
 
}





void CameraCaptureGui::do_spin_led_current_changed(int val)
{  
	if (camera_setting_flag_)
	{ 
		return;
	}
	
	//设置参数时加锁
	camera_setting_flag_ = true;
	if (connected_flag_)
	{

		system_config_param_.led_current = val;

		int ret_code = -1;
		//如果连续采集在用、先暂停
		if (start_timer_flag_)
		{
  
			stopCapturingOneFrameBaseThread();
			 
			
			int ret_code = DfSetSystemConfigParam(system_config_param_);
			if (0 == ret_code)
			{ 
				ui.spinBox_led->setValue(system_config_param_.led_current);
				QString str = QString::fromLocal8Bit("设置投影亮度: ") + QString::number(system_config_param_.led_current);
				addLogMessage(str);
			} 

			do_pushButton_capture_continuous();
		 
		}
		else
		{
			ret_code = DfSetSystemConfigParam(system_config_param_);

			if (0 == ret_code)
			{
				ui.spinBox_led->setValue(system_config_param_.led_current);
				QString str = QString::fromLocal8Bit("设置投影亮度: ") + QString::number(system_config_param_.led_current);
				addLogMessage(str);
			}

		}
		  
	}  

	camera_setting_flag_ = false;
}

void CameraCaptureGui::do_spin_max_z_changed(int val)
{

	processing_gui_settings_data_.Instance().high_z_value = val;


	renderHeightImage(height_map_);
	showImage();

 
}


bool CameraCaptureGui::manyExposureParamHasChanged()
{
	if (system_config_param_.exposure_num != ui.spinBox_exposure_num->value())
	{
		return true;
	}

	for (int i = 0; i < exposure_time_list_.size(); i++)
	{
		if (system_config_param_.exposure_param[i] != exposure_time_list_[i]->value())
		{
			return true;
		}
	}

	return false;
}

bool CameraCaptureGui::setCameraConfigParam()
{
	int ret_code = DfSetParamLedCurrent(system_config_param_.led_current);
	if (0 != ret_code)
	{
		qDebug() << "Set Led Curretn Error;";
		return false;
	}


	ret_code = DfSetParamHdr(system_config_param_.exposure_num, system_config_param_.exposure_param);

	if (0 != ret_code)
	{
		qDebug() << "Set HDR Param Error;";
		return false;
	}

	ret_code = DfSetParamStandardPlaneExternal(&system_config_param_.standard_plane_external_param[0],
		&system_config_param_.standard_plane_external_param[9]);

	if (0 != ret_code)
	{
		qDebug() << "Set Standard Plane Param Error;";
		return false;
	}

	return true;
}

bool CameraCaptureGui::getCameraConfigParam()
{

	int ret_code = DfGetParamLedCurrent(system_config_param_.led_current);
	if (0 != ret_code)
	{
		qDebug() << "Get Led Curretn Error;";
		return false;
	}


	ret_code = DfGetParamHdr(system_config_param_.exposure_num, system_config_param_.exposure_param);

	if (0 != ret_code)
	{
		qDebug() << "Get HDR Param Error;";
		return false;
	}

	ret_code = DfGetParamStandardPlaneExternal(&system_config_param_.standard_plane_external_param[0],
		&system_config_param_.standard_plane_external_param[9]);

	if (0 != ret_code)
	{
		qDebug() << "Get Standard Plane Param Error;";
		return false;
	}


	return true;
}

void CameraCaptureGui::sleep(int sectime)
{
	QTime dieTime = QTime::currentTime().addMSecs(sectime);

	while (QTime::currentTime() < dieTime) {
		QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
	}
}

//更新多曝光参数
void CameraCaptureGui::updateManyExposureParam()
{
	for (int i = 0; i < exposure_time_list_.size(); i++)
	{
		system_config_param_.exposure_param[i] = exposure_time_list_[i]->value();
	}

	system_config_param_.exposure_num = exposure_time_list_.size();


	int ret_code = DfSetSystemConfigParam(system_config_param_);
	if (0 != ret_code)
	{
		qDebug() << "Get Param Error;";
		return;
	}


	//int ret_code = DfSetParamHdr(system_config_param_.exposure_num, system_config_param_.exposure_param);

	//if (0 != ret_code)
	//{
	//	qDebug() << "Set HDR Param Error;";
	//	return;
	//}

	QString str = QString::fromLocal8Bit("同步多曝光参数 ");
	addLogMessage(str);

}

void CameraCaptureGui::do_spin_exposure_num_changed(int val)
{
  
	 
	int item_rows = (val + 1) / 2;

	int item_num = ui.tableWidget_more_exposure->rowCount();


	for (int row = item_num - 1; row >= 0; row--)
	{
		remove_exposure_item(row);
	}
	 
	std::vector<int> old_led_list;
	  

	std::vector<QSpinBox*> old_exposure_time_list = exposure_time_list_;


	for (int i = 0; i < exposure_time_list_.size(); i++)
	{
		old_led_list.push_back(exposure_time_list_.at(i)->value());
		//qDebug()<<"old "<<i<<": "<< old_led_list[i];
	}
	 

	exposure_time_list_.clear();
	 

	for (int i = 0; i < val; i++)
	{
		int rows = i / 2;
		int cols = i % 2;

		//int led_val = system_config_param_.led_current;
		int led_val = system_config_param_.exposure_param[i];
		 
		//if (i < old_exposure_time_list.size())
		//{
		//	//led_val = old_exposure_time_list.at(i)->value();
		//	//qDebug()<<i<< " led_val: " << led_val;

		//	led_val = system_config_param_.exposure_param[i];
		//}

		add_exposure_item(rows, cols, led_val);
	}

	//qDebug() << "exposure_time_list size: " << exposure_time_list_.size();
	ui.tableWidget_more_exposure->repaint();
	ui.verticalLayout->update();
}

/*************************************************************************************************************************************/

bool CameraCaptureGui::connectCamera(QString ip)
{

	return true;
}


bool CameraCaptureGui::captureBrightness()
{
	if (!connected_flag_)
	{
		return false;
	}

	int width = camera_width_;
	int height = camera_height_; 

 
	 

	int image_size = width * height;

	cv::Mat brightness(cv::Size(width, height), CV_8U,cv::Scalar(0));
	unsigned char* brightness_buf = (unsigned char*)brightness.data;

	int ret_code = DfGetCameraData(0, 0,brightness_buf, image_size,0, 0,0, 0);
	 

	if (0 == ret_code)
	{
		brightness_map_ = brightness.clone();
		return true;
	}

	return false;
}


bool CameraCaptureGui::stopCapturingOneFrameBaseThread()
{
	do_pushButton_capture_continuous();

	for (int i = 0; i < 1000; i++)
	{
		sleep(10);
		if (!capturing_flag_)
		{
			//qDebug() << "capturing_flag_: " << capturing_flag_;
			return true;
		}

	}

	return false;
}

void CameraCaptureGui::captureOneFrameBaseThread(bool hdr)
{

	if (!connected_flag_)
	{
		return ;
	}


	if (capturing_flag_)
	{
		return;
	}
	capturing_flag_ = true;

	int width = camera_width_;
	int height = camera_height_; 

	int image_size = width * height;

	cv::Mat brightness(height, width, CV_8U, cv::Scalar(0));
	cv::Mat depth(height, width, CV_32F, cv::Scalar(0.));
	cv::Mat point_cloud(height, width, CV_32FC3, cv::Scalar(0.));

	int depth_buf_size = image_size * 1 * 4;
	int brightness_bug_size = image_size;

	int ret_code = -1;

	if (hdr)
	{
		ret_code = DfGetFrameHdr((float*)depth.data, depth_buf_size, (uchar*)brightness.data, brightness_bug_size);
	}
	else
	{
		ret_code = DfGetFrame03((float*)depth.data, depth_buf_size, (uchar*)brightness.data, brightness_bug_size);
	}
	 

	/***************************************************************************/
	if (0 == ret_code)
	{

		float temperature = 0;
		ret_code = DfGetDeviceTemperature(temperature);
		std::cout << "temperature: " << temperature << std::endl;
		emit send_temperature_update(temperature);

		//capture_m_mutex_.lock();

		brightness_map_ = brightness.clone();
		depth_map_ = depth.clone();

		depthTransformPointcloud((float*)depth.data, (float*)point_cloud.data);
		transformPointcloud((float*)point_cloud.data, (float*)point_cloud.data, system_config_param_.standard_plane_external_param, &system_config_param_.standard_plane_external_param[9]);

		std::vector<cv::Mat> channels;
		cv::split(point_cloud, channels);
		height_map_ = channels[2].clone();
		 
		//capture_m_mutex_.unlock();


		capture_show_flag_ = true;


		emit send_images_update();

		  
	} 
	else
	{
		start_timer_flag_ = false;
	}
	 
	capturing_flag_ = false;
}

bool CameraCaptureGui::captureOneFrameData()
{

	if (!connected_flag_)
	{
		return false;
	}

	int width = camera_width_;
	int height = camera_height_;
  

	addLogMessage(QString::fromLocal8Bit("采集数据："));  
	/*****************************************************************************/
	int image_size = width * height;

	cv::Mat brightness(height, width, CV_8U, cv::Scalar(0));
	cv::Mat depth(height, width, CV_32F, cv::Scalar(0.));
	cv::Mat point_cloud(height, width, CV_32FC3, cv::Scalar(0.)); 

	int depth_buf_size = image_size * 1 * 4;  
	int brightness_bug_size = image_size; 

	int ret_code = 0;

	if (ui.checkBox_hdr->isChecked())
	{ 
		if (connected_flag_)
		{
			bool changed = manyExposureParamHasChanged();

			if (changed)
			{
				updateManyExposureParam();
			}
		}

		ret_code = DfGetFrameHdr((float*)depth.data, depth_buf_size, (uchar*)brightness.data, brightness_bug_size);
	}
	else
	{ 
		ret_code = DfGetFrame03((float*)depth.data, depth_buf_size, (uchar*)brightness.data, brightness_bug_size);
	}
	  
	/***************************************************************************/ 
	if (0 == ret_code)
	{
		brightness_map_ = brightness.clone();
		depth_map_ = depth.clone();
		 
		depthTransformPointcloud((float*)depth.data, (float*)point_cloud.data);  
		transformPointcloud((float*)point_cloud.data,(float*)point_cloud.data, system_config_param_.standard_plane_external_param, &system_config_param_.standard_plane_external_param[9]);
	 
		std::vector<cv::Mat> channels;
		cv::split(point_cloud, channels);
		height_map_ = channels[2].clone();
		

		addLogMessage(QString::fromLocal8Bit("采集完成！"));
  
		float temperature = 0;
		ret_code = DfGetDeviceTemperature(temperature);


		emit send_temperature_update(temperature); 

		return true;
	}

	addLogMessage(QString::fromLocal8Bit("采集失败！"));
	return false;
	   
}
 

bool CameraCaptureGui::isConnect()
{
	return connected_flag_;
}


void  CameraCaptureGui::do_pushButton_connect()
{ 

	if (!connected_flag_)
	{

		camera_ip_ = ui.lineEdit_ip->text();
		if (camera_ip_.isEmpty())
		{
			addLogMessage(QString::fromLocal8Bit("请设置IP！"));
			return;
		}
		  
		  

		addLogMessage(QString::fromLocal8Bit("连接相机："));
		int ret_code = DfConnect(camera_ip_.toStdString().c_str());

		if (0 == ret_code)
		{
			//必须连接相机成功后，才可获取相机分辨率
			ret_code = DfGetCameraResolution(&camera_width_, &camera_height_);
			std::cout << "Width: " << camera_width_ << "    Height: " << camera_height_ << std::endl;

			if (0 != ret_code)
			{
				qDebug() << "Connect Error!;";
				return;
			}
			//获取相机标定参数
			DfGetCalibrationParam(camera_calibration_param_);
	 

			addLogMessage(QString::fromLocal8Bit("连接相机成功！"));
			//保存ip配置
			processing_gui_settings_data_.Instance().ip = camera_ip_;

			//ret_code = DfGetSystemConfigParam(system_config_param_);
			//if (0 != ret_code)
			//{
			//	qDebug() << "Get Param Error;";
			//	//return;
			//}

			ret_code = DfSetSystemConfigParam(system_config_param_);
			if (0 != ret_code)
			{
				qDebug() << "Set Param Error;";
				//return;
			}

			undateSystemConfigUiData();
		}
		else
		{
			std::cout << "Connect Camera Error!";
			addLogMessage(QString::fromLocal8Bit("连接相机失败！"));
			return;
		}


		connected_flag_ = true;

		CalibrationParam calib_param;
		ret_code = DfGetCalibrationParam(&calib_param);

		if (0 == ret_code)
		{
			std::cout << "intrinsic: " << std::endl;
			for (int r = 0; r < 3; r++)
			{
				for (int c = 0; c < 3; c++)
				{
					std::cout << calib_param.intrinsic[3 * r + c] << "\t";
				}
				std::cout << std::endl;
			}

			std::cout << "extrinsic: " << std::endl;
			for (int r = 0; r < 4; r++)
			{
				for (int c = 0; c < 4; c++)
				{
					std::cout << calib_param.extrinsic[4 * r + c] << "\t";
				}
				std::cout << std::endl;
			}

			std::cout << "distortion: " << std::endl;
			for (int r = 0; r < 1; r++)
			{
				for (int c = 0; c < 12; c++)
				{
					std::cout << calib_param.distortion[1 * r + c] << "\t";
				}
				std::cout << std::endl;
			}
		}
		else
		{
			std::cout << "Get Calibration Data Error!";
			return;
		}

		ui.pushButton_connect->setIcon(QIcon(":/dexforce_camera_gui/image/disconnect.png"));
	}
	else
	{
		//断开相机
		do_pushButton_disconnect();

	}



}

void  CameraCaptureGui::do_pushButton_disconnect()
{
	
	if (connected_flag_)
	{
		if (start_timer_flag_)
		{
			stopCapturingOneFrameBaseThread();
		}

		DfDisconnect(camera_ip_.toStdString().c_str());
		addLogMessage(QString::fromLocal8Bit("断开相机！")); 
		connected_flag_ = false;

		ui.pushButton_connect->setIcon(QIcon(":/dexforce_camera_gui/image/connect.png"));
	}


}


double CameraCaptureGui::computePointsDistance(cv::Point2f p_0, cv::Point2f p_1, cv::Mat point_cloud)
{
  
	std::vector<cv::Mat> point_cloud_channels;
	cv::split(point_cloud, point_cloud_channels); 

	//插值
	cv::Point3f f_p_0_inter, f_p_1_inter;

	Calibrate_Function calib_function;
	f_p_0_inter.x = calib_function.Bilinear_interpolation(p_0.x, p_0.y, point_cloud_channels[0]);
	f_p_0_inter.y = calib_function.Bilinear_interpolation(p_0.x, p_0.y, point_cloud_channels[1]);
	f_p_0_inter.z = calib_function.Bilinear_interpolation(p_0.x, p_0.y, point_cloud_channels[2]);

	f_p_1_inter.x = calib_function.Bilinear_interpolation(p_1.x, p_1.y, point_cloud_channels[0]);
	f_p_1_inter.y = calib_function.Bilinear_interpolation(p_1.x, p_1.y, point_cloud_channels[1]);
	f_p_1_inter.z = calib_function.Bilinear_interpolation(p_1.x, p_1.y, point_cloud_channels[2]);

	cv::Point3f differ_p = f_p_1_inter - f_p_0_inter;
	double differ_val = std::sqrtf(differ_p.x * differ_p.x + differ_p.y * differ_p.y + differ_p.z * differ_p.z);

	return differ_val;
} 

void CameraCaptureGui::do_pushButton_calibrate_external_param()
{

	if (depth_map_.empty() || brightness_map_.empty())
	{
		return;
	} 
	 
	   
	cv::Mat cameraMatrix(3, 3, CV_32FC1, camera_calibration_param_.camera_intrinsic);
	cv::Mat distCoeffs = cv::Mat(5, 1, CV_32F, camera_calibration_param_.camera_distortion);  

	if (true)
	{
		//ICP
		cv::Mat undist_img;
		cv::undistort(brightness_map_, undist_img, cameraMatrix, distCoeffs);


		Calibrate_Function calib_function;
		std::vector<cv::Point2f> undist_circle_points;
		bool found = calib_function.findCircleBoardFeature(undist_img, undist_circle_points);

		if (!found)
		{
			return;
		}


		cv::medianBlur(depth_map_, depth_map_, 3);

		cv::Mat points_map(brightness_map_.size(), CV_32FC3, cv::Scalar(0., 0., 0.));
		depthTransformPointcloud((float*)depth_map_.data, (float*)points_map.data);

		/*******************************************************************************************/
		std::vector<cv::Point3f> point_3d;
		bilinearInterpolationFeaturePoints(undist_circle_points, point_3d, points_map);

		PrecisionTest precision_machine;
		cv::Mat pc1(point_3d.size(), 3, CV_64F, cv::Scalar(0));
		cv::Mat pc2(point_3d.size(), 3, CV_64F, cv::Scalar(0));

		std::vector<cv::Point3f> world_points = calib_function.generateAsymmetricWorldFeature(20, 10);

		for (int r = 0; r < point_3d.size(); r++)
		{
			pc2.at<double>(r, 0) = point_3d[r].x;
			pc2.at<double>(r, 1) = point_3d[r].y;
			pc2.at<double>(r, 2) = point_3d[r].z;
		}
		for (int r = 0; r < world_points.size(); r++)
		{
			pc1.at<double>(r, 0) = world_points[r].x;
			pc1.at<double>(r, 1) = world_points[r].y;
			pc1.at<double>(r, 2) = world_points[r].z;
		}

		cv::Mat r(3, 3, CV_64F, cv::Scalar(0));
		cv::Mat t(3, 3, CV_64F, cv::Scalar(0));

		precision_machine.svdIcp(pc1, pc2, r, t);  
		r.convertTo(r, CV_32F);
		t.convertTo(t, CV_32F);
		transformPointcloud((float*)points_map.data, (float*)points_map.data, (float*)r.data, (float*)t.data);

  
		std::vector<cv::Mat> channels;
		cv::split(points_map, channels); 
		cv::Mat height_map = channels[2].clone(); 

		height_map_ = height_map.clone();
		renderHeightImage(height_map);
		showImage();

		ui.spinBox_min_z->setValue(-10);
		ui.spinBox_max_z->setValue(10);

		for (int i = 0; i < 9; i++)
		{
			system_config_param_.standard_plane_external_param[i] = r.ptr<float>(0)[i];
			//qDebug() << system_config_param_.external_param[i];
		}

		for (int i = 0; i < 3; i++)
		{
			system_config_param_.standard_plane_external_param[9 + i] = t.ptr<float>(0)[i];
			//qDebug() << translation_mat.ptr<float>(0)[i];
		} 
		if (connected_flag_)
		{
			int ret_code = DfSetSystemConfigParam(system_config_param_);
			if (0 != ret_code)
			{
				qDebug() << "Get Param Error;";
				return;
			} 
			QString str = QString::fromLocal8Bit("保存高度映射基准平面");
			addLogMessage(str); 

		}
		else
		{
			QString str = QString::fromLocal8Bit("相机已断连！");
			addLogMessage(str);
		} 
	}

	/**************************************************************************************************/ 
}

void CameraCaptureGui::do_pushButton_test_accuracy()
{
	Calibrate_Function calib_function;

	if (brightness_map_.empty() || brightness_map_.type() != CV_8UC1)
	{
		return;
	}

	cv::Mat cameraMatrix(3, 3, CV_32FC1, camera_calibration_param_.camera_intrinsic);
	cv::Mat distCoeffs = cv::Mat(5, 1, CV_32F, camera_calibration_param_.camera_distortion);


	cv::Mat img = brightness_map_.clone();
	cv::Mat undist_img;
	cv::undistort(brightness_map_, undist_img, cameraMatrix, distCoeffs);



	/*******************************************************************************/
	//PNP
	if (false)
	{
		/*************************************************************************************/
		//PNP精度

		std::vector<cv::Point2f> circle_points;
		bool found = calib_function.findCircleBoardFeature(img, circle_points);

		if (found)
		{
			Calibrate_Function calib_machine;
			std::vector<cv::Point3f> objects = calib_machine.generateAsymmetricWorldFeature(20.0, 10.0);

			cv::Mat raux, taux;
			std::vector<cv::Point2f> image_points_pro;

			cv::solvePnP(objects, circle_points, cameraMatrix, distCoeffs, raux, taux, false, cv::SOLVEPNP_EPNP);
			cv::projectPoints(objects, raux, taux, cameraMatrix, distCoeffs, image_points_pro);   //通过得到的摄像机内外参数，对角点的空间三维坐标进行重新投影计算
			double err = cv::norm(cv::Mat(circle_points), cv::Mat(image_points_pro), cv::NORM_L2);
			//addLogMessage(QString::fromLocal8Bit("NORM_L2: ") + QString::number(err));

			/*********************************************************************************/
			//平均像素距离
			double sumvalue = 0.0;
			for (size_t i = 0; i < image_points_pro.size(); i++)
			{
				double x = image_points_pro[i].x - circle_points[i].x;
				double y = image_points_pro[i].y - circle_points[i].y;
				sumvalue += sqrt(x * x + y * y);
			}
			sumvalue /= image_points_pro.size();
			addLogMessage(QString::fromLocal8Bit("偏差: ") + QString::number(sumvalue, 'f', 5) + QString::fromLocal8Bit(" 像素"));
			/*********************************************************************************/

			cv::Mat color_img;
			cv::Size board_size = calib_function.getBoardSize();
			cv::cvtColor(brightness_map_, color_img, cv::COLOR_GRAY2BGR);
			cv::drawChessboardCorners(color_img, board_size, circle_points, found);
			//cv::circle(color_img, circle_points[0], 15, cv::Scalar(0, 255, 0), 2);
			//cv::circle(color_img, circle_points[6], 20, cv::Scalar(0, 255, 0), 2);

			render_image_brightness_ = color_img.clone();
			showImage();
		}


		/*************************************************************************************/
	}


	/*****************************************************************************************************/
	//平台测试精度
	if (false)
	{

		PrecisionTest p_test_machine;

		std::vector<cv::Point2f> circle_points;
		bool found = calib_function.findCircleBoardFeature(undist_img, circle_points);

		if (!found)
		{
			return;
		}
		cv::Mat points_map(brightness_map_.size(), CV_32FC3, cv::Scalar(0., 0., 0.));
		depthTransformPointcloud((float*)depth_map_.data, (float*)points_map.data);


		cv::Mat color_img;
		cv::Size board_size = calib_function.getBoardSize();
		cv::cvtColor(brightness_map_, color_img, cv::COLOR_GRAY2BGR);
		cv::drawChessboardCorners(color_img, board_size, circle_points, found);
		render_image_brightness_ = color_img.clone();
		showImage();

		/*******************************************************************************************/
		std::vector<cv::Point3f> point_3d;
		bilinearInterpolationFeaturePoints(circle_points, point_3d, points_map);



		//QString StrCurrentTime = last_path_ + "/feature_points_" + QDateTime::currentDateTime().toString("yyyy-MM-dd_hh-mm-ss"); 
		//StrCurrentTime += ".ply"; 
		//FileIoFunction file_io_machine;
		//file_io_machine.SaveAsciiPointsToPly(point_3d, StrCurrentTime); 
		//addLogMessage(QString::fromLocal8Bit("保存圆点: ") + StrCurrentTime);



		std::vector<float> plane;
		cv::Point3f center_point;

		float rms = p_test_machine.fitPlaneBaseLeastSquares(point_3d, plane, center_point);

		//addLogMessage(QString::fromLocal8Bit("平面拟合RMS: ") + QString::number(rms));


		center_points_list_.push_back(center_point);
		rms_list_.push_back(rms);
		plane_list_.push_back(plane);
		feature_points_list_.push_back(point_3d);


		if (center_points_list_.size() > 1)
		{
			float dist = p_test_machine.computePointToPlaneDistance(center_points_list_[center_points_list_.size() - 1],
				plane_list_[center_points_list_.size() - 2]);

			//float value = std::floorf(dist);


			addLogMessage(QString::fromLocal8Bit("平面拟合RMS: ") + QString::number(rms) +
				QString::fromLocal8Bit("距离: ") + QString::number(dist));
		}
		else
		{
			addLogMessage(QString::fromLocal8Bit("平面拟合RMS: ") + QString::number(rms) +
				QString::fromLocal8Bit("距离: ") + QString::number(0));
		}

	}


	/*********************************************************************************************/
	//icp
	if (true)
	{
		//ICP
		cv::Mat undist_img;
		cv::undistort(brightness_map_, undist_img, cameraMatrix, distCoeffs); 
		Calibrate_Function calib_function;
		std::vector<cv::Point2f> undist_circle_points;
		bool found = calib_function.findCircleBoardFeature(undist_img, undist_circle_points);

		if (!found)
		{
			return;
		}

		//显示
		if (true)
		{
			cv::Mat color_img;
			cv::Size board_size = calib_function.getBoardSize();
			cv::cvtColor(brightness_map_, color_img, cv::COLOR_GRAY2BGR);
			cv::drawChessboardCorners(color_img, board_size, undist_circle_points, found);
			render_image_brightness_ = color_img.clone();
			showImage();
		}


		cv::medianBlur(depth_map_, depth_map_, 3);
		cv::Mat points_map(brightness_map_.size(), CV_32FC3, cv::Scalar(0., 0., 0.));
		depthTransformPointcloud((float*)depth_map_.data, (float*)points_map.data);

		/*******************************************************************************************/
		std::vector<cv::Point3f> point_3d;
		bilinearInterpolationFeaturePoints(undist_circle_points, point_3d, points_map);

		PrecisionTest precision_machine;
		cv::Mat pc1(point_3d.size(), 3, CV_64F, cv::Scalar(0));
		cv::Mat pc2(point_3d.size(), 3, CV_64F, cv::Scalar(0));

		std::vector<cv::Point3f> world_points = calib_function.generateAsymmetricWorldFeature(20, 10);

		for (int r = 0; r < point_3d.size(); r++)
		{
			pc2.at<double>(r, 0) = point_3d[r].x;
			pc2.at<double>(r, 1) = point_3d[r].y;
			pc2.at<double>(r, 2) = point_3d[r].z;
		}
		for (int r = 0; r < world_points.size(); r++)
		{
			pc1.at<double>(r, 0) = world_points[r].x;
			pc1.at<double>(r, 1) = world_points[r].y;
			pc1.at<double>(r, 2) = world_points[r].z;
		}

		cv::Mat r(3, 3, CV_64F, cv::Scalar(0));
		cv::Mat t(3, 3, CV_64F, cv::Scalar(0));

		precision_machine.svdIcp(pc1, pc2, r, t);

		std::vector<cv::Point3f> transform_points;

		precision_machine.transformPoints(point_3d, transform_points, r, t);

		double diff = precision_machine.computeTwoPointSetDistance(world_points, transform_points);

		//addLogMessage(QString::fromLocal8Bit("平均点偏差: ") + QString::number(diff) +
		//	QString::fromLocal8Bit(" 距离: ") + QString::number(0));


		//计算平面距离

		std::vector<float> plane;
		cv::Point3f center_point; 
		float rms = precision_machine.fitPlaneBaseLeastSquares(point_3d, plane, center_point);
		  

		center_points_list_.push_back(center_point);
		rms_list_.push_back(rms);
		plane_list_.push_back(plane);
		feature_points_list_.push_back(point_3d);


		if (center_points_list_.size() > 1)
		{
			float dist = precision_machine.computePointToPlaneDistance(center_points_list_[center_points_list_.size() - 1],
				plane_list_[center_points_list_.size() - 2]);
			 

			addLogMessage(QString::fromLocal8Bit("标定精度: ") + QString::number(diff) +
				QString::fromLocal8Bit("	距离: ") + QString::number(dist));
		}
		else
		{
			addLogMessage(QString::fromLocal8Bit("标定精度: ") + QString::number(diff) +
				QString::fromLocal8Bit("	距离: ") + QString::number(0));
		}



	}

	/**********************************************************************************************/
	//点距离
	if (false)
	{

		std::vector<cv::Point2f> circle_points;
		bool found = calib_function.findCircleBoardFeature(undist_img, circle_points);

		if (!found)
		{
			return;
		}
		cv::Mat points_map(brightness_map_.size(), CV_32FC3, cv::Scalar(0., 0., 0.));
		depthTransformPointcloud((float*)depth_map_.data, (float*)points_map.data);


		cv::Point2f f_p_0_0 = circle_points[0];
		cv::Point2f f_p_0_1 = circle_points[6];

		double dist_0 = computePointsDistance(f_p_0_0, f_p_0_1, points_map) - 120.0;

		double max_err = std::abs(dist_0);

		QString dist_str = QString::number(dist_0);

		cv::Point2f f_p_1_0 = circle_points[6];
		cv::Point2f f_p_1_1 = circle_points[76];

		double dist_1 = computePointsDistance(f_p_1_0, f_p_1_1, points_map) - 100.0;
		dist_str += " , ";
		dist_str += QString::number(dist_1);

		if (std::abs(dist_1) > max_err)
		{
			max_err = std::abs(dist_1);
		}

		cv::Point2f f_p_2_0 = circle_points[76];
		cv::Point2f f_p_2_1 = circle_points[70];

		double dist_2 = computePointsDistance(f_p_2_0, f_p_2_1, points_map) - 120.0;
		dist_str += " , ";
		dist_str += QString::number(dist_2);

		if (std::abs(dist_2) > max_err)
		{
			max_err = std::abs(dist_2);
		}

		cv::Point2f f_p_3_0 = circle_points[70];
		cv::Point2f f_p_3_1 = circle_points[0];

		double dist_3 = computePointsDistance(f_p_3_0, f_p_3_1, points_map) - 100.0;
		dist_str += " , ";
		dist_str += QString::number(dist_3);

		if (std::abs(dist_3) > max_err)
		{
			max_err = std::abs(dist_3);
		}

		//addLogMessage(QString::fromLocal8Bit("标定板距离：") + dist_str);
		addLogMessage(QString::fromLocal8Bit("偏差：") + QString::number(max_err) + "mm");

	}
}

void CameraCaptureGui::do_pushButton_save_as()
{


	QString StrCurrentTime = "/" + QDateTime::currentDateTime().toString("yyyy-MM-dd_hh-mm-ss");

	QString path = QFileDialog::getSaveFileName(this, "Set Save Name",last_path_ + StrCurrentTime);

	if (path.isEmpty())
	{
		return;
	}

	last_path_ = path;

	//QStringList str_list = path.split(".");

	//QString name = str_list[0];
	 

	std::thread t_s(&CameraCaptureGui::saveOneFrameData,this, path);
	t_s.detach(); 
	addLogMessage(QString::fromLocal8Bit("保存路径：")+ path);
	 

}


bool CameraCaptureGui::captureOneFrameAndRender()
{
	bool ret = captureOneFrameData(); 


	if (ret)
	{

		renderBrightnessImage(brightness_map_);
		renderDepthImage(depth_map_);
		renderHeightImage(height_map_);
		showImage();

		if (ui.checkBox_auto_save->isChecked())
		{
			QString StrCurrentTime = "/" + QDateTime::currentDateTime().toString("yyyy-MM-dd_hh-mm-ss");
			QString path_name = sys_path_ + StrCurrentTime;
			//saveOneFrameData(path_name);
			  
			std::thread t_s(&CameraCaptureGui::saveOneFrameData, this, path_name);
			t_s.detach();
			addLogMessage(QString::fromLocal8Bit("保存路径：") + path_name);
		}
	}
	else
	{
		return false;
	}

	return true;
}


void  CameraCaptureGui::do_pushButton_capture_one_frame()
{
	if (!connected_flag_)
	{
		do_pushButton_connect();
		captureOneFrameAndRender();
		do_pushButton_disconnect();
		return;
	}

 

	captureOneFrameAndRender();
	 




}
 

void CameraCaptureGui::do_undate_show_slot()
{

	//capture_m_mutex_.lock();

	if (!capture_show_flag_)
	{
		return;
	}

	renderBrightnessImage(brightness_map_);
	renderDepthImage(depth_map_);
	renderHeightImage(height_map_);

	showImage(); 

	capture_show_flag_ = false;

	//capture_m_mutex_.unlock();
	if (ui.checkBox_auto_save->isChecked())
	{
		QString StrCurrentTime = "/" + QDateTime::currentDateTime().toString("yyyy-MM-dd_hh-mm-ss");
		QString path_name = sys_path_ + StrCurrentTime;
		//saveOneFrameData(path_name);

		std::thread t_s(&CameraCaptureGui::saveOneFrameData, this, path_name);
		t_s.detach();
		addLogMessage(QString::fromLocal8Bit("保存路径：") + path_name);
	}

  
}


void  CameraCaptureGui::do_timeout_capture_slot()
{

	std::cout << "capture timeout"<<std::endl;

	capture_timer_.stop();

	if (start_timer_flag_ && connected_flag_)
	{  
			//bool ret = captureOneFrameAndRender();  
			//if (!ret)
			//{
			//	//停止连续采集
			//	do_pushButton_capture_continuous();
			//} 
			//else
			//{
			//	capture_timer_.start();
			//}

			std::thread t_s(&CameraCaptureGui::captureOneFrameBaseThread, this, ui.checkBox_hdr->isChecked());
			t_s.detach();  
			capture_timer_.start();
	}
	else
	{	
		//	//停止连续采集
			do_pushButton_capture_continuous();

	}

}

void  CameraCaptureGui::do_pushButton_capture_many_frame()
{


}

void  CameraCaptureGui::do_pushButton_capture_continuous()
{

	if (start_timer_flag_)
	{
		start_timer_flag_ = false;
		capture_timer_.stop(); 

		ui.pushButton_capture_continuous->setIcon(QIcon(":/dexforce_camera_gui/image/video_start.png"));
	}
	else
	{
		if (connected_flag_)
		{
			start_timer_flag_ = true;
			capturing_flag_ = false;
			capture_timer_.start();
			ui.pushButton_capture_continuous->setIcon(QIcon(":/dexforce_camera_gui/image/video_stop.png"));
		}
		 
	}

	//capture_thread_ = new QThread(this);
	//capture_thread_->start();

	//capture_timer_ = new QTimer(0);
	//capture_timer_->setInterval(100);
	//capture_timer_->moveToThread(capture_thread_);
	//connect(capture_timer_, SIGNAL(timeout()), this, SLOT(workSlot()), Qt::DirectConnection);
	//connect(capture_thread_, SIGNAL(started()), capture_timer_, SLOT(start()));


	//disconnect(capture_timer_, SIGNAL(timeout()), this, SLOT(workSlot()));
	//disconnect(capture_thread_, SIGNAL(started()), capture_timer_, SLOT(start()));
	//capture_timer_->stop();
	//delete capture_timer_;
}

/*********************************************************************************************************************************/
//HDR显示

void CameraCaptureGui::do_checkBox_toggled_hdr(bool state)
{

	if (connected_flag_)
	{
		if (state)
		{

			bool changed = manyExposureParamHasChanged();

			if (changed)
			{
				if (start_timer_flag_)
				{
					stopCapturingOneFrameBaseThread();
					updateManyExposureParam();
					do_pushButton_capture_continuous();
				}
				else
				{

					updateManyExposureParam();
				}

			}
		}

	}
	 
}


/*****************************************************************************************************************************/
//显示相关
void CameraCaptureGui::do_QRadioButton_toggled_brightness(bool state)
{
	if (state)
	{
		radio_button_flag_ = SELECT_BRIGHTNESS_FLAG_;
		//qDebug() << "state: " << radio_button_flag_;
		showImage();
	}
}

void CameraCaptureGui::do_QRadioButton_toggled_color_depth(bool state)
{
	if (state)
	{
		radio_button_flag_ = SELECT_COLOR_DEPTH_FLAG_;
		//qDebug() << "state: " << radio_button_flag_;
		showImage();
	}
}

void CameraCaptureGui::do_QRadioButton_toggled_gray_depth(bool state)
{
	if (state)
	{
		radio_button_flag_ = SELECT_HEIGHT_MAP_FLAG_;
		//qDebug() << "state: " << radio_button_flag_;
		showImage();
	}
}



bool CameraCaptureGui::showImage()
{
	switch (radio_button_flag_)
	{
	case 1:
	{
		setImage(render_image_brightness_);
	}
	break;

	case 2:
	{
		setImage(render_image_color_height_);
	}
	break;

	case 3:
	{
		setImage(render_image_color_depth_);
	}
	break;

	default:
		break;
	}

	return true;
}

bool CameraCaptureGui::setImage(cv::Mat img)
{
	cv::Mat show_img = img.clone();

	if (img.empty())
	{
		return false;
	}

	QImage qimg;

	if (show_img.channels() == 3)//RGB Img
	{
		cv::Mat Rgb;
		cv::cvtColor(show_img, Rgb, cv::COLOR_BGR2RGB);//颜色空间转换
		//qimg = QImage((const uchar*)(Rgb.data), Rgb.cols, Rgb.rows, Rgb.step, QImage::Format_RGB888);

		qimg = QImage(show_img.data, show_img.cols, show_img.rows, show_img.step, QImage::Format_RGB888);

	}
	else//Gray Img
	{
		if (CV_8UC1 == show_img.type())
		{
			qimg = QImage((const uchar*)(show_img.data), show_img.cols, show_img.rows, show_img.cols * show_img.channels(), QImage::Format_Indexed8);
		}
		else if (CV_32FC1 == show_img.type())
		{

			show_img.convertTo(show_img, CV_8U);

			qimg = QImage((const uchar*)(show_img.data), show_img.cols, show_img.rows, show_img.cols * show_img.channels(), QImage::Format_Indexed8);

		}

	}



	//pixmap_ = QPixmap::fromImage(qimg);
	 

	ui.label_image->setPixmap(QPixmap::fromImage(qimg));
	ui.label_image->setScaledContents(true);
	//ui.label_image->setPixmap(pixmap_.scaled(ui.label_image->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));  // 保持比例 & 平滑缩放(无锯齿)

	return true;
}


/************************************************************************************************************************************/

bool CameraCaptureGui::bilinearInterpolationFeaturePoints(std::vector<cv::Point2f> feature_points, std::vector<cv::Point3f>& point_3d, cv::Mat point_cloud)
{
	if (point_cloud.empty())
		return false;


	Calibrate_Function calib_function;
	std::vector<cv::Mat> point_cloud_channels;
	cv::split(point_cloud, point_cloud_channels);

	point_3d.clear();

	for (int i = 0; i < feature_points.size(); i++)
	{
		cv::Point2f p_0 = feature_points[i];
		cv::Point3f f_p_inter;

		f_p_inter.x = calib_function.Bilinear_interpolation(p_0.x, p_0.y, point_cloud_channels[0]);
		f_p_inter.y = calib_function.Bilinear_interpolation(p_0.x, p_0.y, point_cloud_channels[1]);
		f_p_inter.z = calib_function.Bilinear_interpolation(p_0.x, p_0.y, point_cloud_channels[2]);

		point_3d.push_back(f_p_inter);
	}


	return true;
}

/*********************************************************************************************************************/