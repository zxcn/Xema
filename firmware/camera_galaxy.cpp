#include "camera_galaxy.h"
#include "easylogging++.h"

CameraGalaxy::CameraGalaxy()
{

}
CameraGalaxy::~CameraGalaxy()
{

}


bool CameraGalaxy::grap(unsigned char* buf)
{

    LOG(INFO) << "capture:";
    GX_STATUS status = GXDQBuf(hDevice_, &pFrameBuffer_, 1000);
    LOG(INFO) << "status=" << status;
    if (status != GX_STATUS_SUCCESS)
    {
        status = GXQBuf(hDevice_, pFrameBuffer_);
        return false;
    }

    if (pFrameBuffer_->nStatus == GX_FRAME_STATUS_SUCCESS)
    {
        int img_rows = pFrameBuffer_->nHeight;
        int img_cols = pFrameBuffer_->nWidth;
        int img_size = img_rows * img_cols;

        memcpy(buf, pFrameBuffer_->pImgBuf, img_size);
    }

    status = GXQBuf(hDevice_, pFrameBuffer_);
    if (status != GX_STATUS_SUCCESS)
    { 
        return false;
    }

    return true;
}

bool CameraGalaxy::streamOn()
{
    GX_STATUS status = GX_STATUS_SUCCESS;

    while (!stream_mutex_.try_lock_for(std::chrono::milliseconds(1)))
    {
        LOG(INFO) << "GXStreamOn --";
    }

    status = GXSetEnum(hDevice_, GX_ENUM_TRIGGER_MODE, GX_TRIGGER_MODE_ON);
    if (status != GX_STATUS_SUCCESS)
    {

        LOG(INFO) << "GXSetEnum Error: " << status;
        return false;
    }

    LOG(INFO) << "GXStreamOn";
    status = GXStreamOn(hDevice_);
    if (status != GX_STATUS_SUCCESS)
    {

        LOG(INFO) << "Stream On Error: " << status;
        return false;
    }

    stream_mutex_.unlock();

    return true;
}


void CameraGalaxy::streamOffThread()
{
    while (!stream_mutex_.try_lock_for(std::chrono::milliseconds(1)))
    {
        LOG(INFO) << "GXStreamOff --";
    }

    GXStreamOff(hDevice_);
    LOG(INFO) << "Thread GXStreamOff";
    stream_mutex_.unlock();
}

bool CameraGalaxy::streamOff()
{
    // std::thread stop_thread(GXStreamOff, hDevice_);
    // stop_thread.detach();
    
    std::thread stop_thread(&CameraGalaxy::streamOffThread,this);
    stop_thread.detach();
    return true;
}
 

bool CameraGalaxy::openCamera()
{

    GX_STATUS status = GX_STATUS_SUCCESS;
    uint32_t nDeviceNum = 0;

    status = GXInitLib();
    if (status != GX_STATUS_SUCCESS)
    {
        return false;
    }

    status = GXUpdateDeviceList(&nDeviceNum, 1000);
    if ((status != GX_STATUS_SUCCESS) || (nDeviceNum <= 0))
    {
        return false;
    }

    char cam_idx[8] = "0";
    if (status == GX_STATUS_SUCCESS && nDeviceNum > 0)
    {
        GX_DEVICE_BASE_INFO *pBaseinfo = new GX_DEVICE_BASE_INFO[nDeviceNum];
        size_t nSize = nDeviceNum * sizeof(GX_DEVICE_BASE_INFO);
        // Gets the basic information of all devices.
        status = GXGetAllDeviceBaseInfo(pBaseinfo, &nSize);
	for(int i=0; i<nDeviceNum; i++)
	{
	    if(GX_DEVICE_CLASS_U3V == pBaseinfo[i].deviceClass)
	    {
		//camera index starts from 1
		snprintf(cam_idx, 8, "%d", i+1);
	    }
	}

        delete []pBaseinfo;
    }

    
    GX_OPEN_PARAM stOpenParam;
    stOpenParam.accessMode = GX_ACCESS_EXCLUSIVE;
    stOpenParam.openMode = GX_OPEN_INDEX;
    stOpenParam.pszContent = cam_idx;
    status = GXOpenDevice(&stOpenParam, &hDevice_);
   
    if (status != GX_STATUS_SUCCESS)
    {
   
	    LOG(INFO)<<"Open Camera Error!";
	    return false;
    }

 
    if (status == GX_STATUS_SUCCESS)
    {

        /***********************************************************************************************/
        //�� �� �� �� ֵ
        status = GXSetFloat(hDevice_, GX_FLOAT_EXPOSURE_TIME, 12000.0);
        //�� �� �� �� �� �� �� ��
        status = GXSetEnum(hDevice_, GX_ENUM_EXPOSURE_AUTO, GX_EXPOSURE_AUTO_OFF);

        //ѡ �� �� �� ͨ �� �� ��
        status = GXSetEnum(hDevice_, GX_ENUM_GAIN_SELECTOR, GX_GAIN_SELECTOR_ALL); 
        //�� �� �� �� ֵ
        status = GXSetFloat(hDevice_, GX_FLOAT_GAIN, 0.0); 
        //�� �� �� �� ѡ �� Ϊ Line2
        status = GXSetEnum(hDevice_, GX_ENUM_LINE_SELECTOR, GX_ENUM_LINE_SELECTOR_LINE2);
        //�� �� �� �� �� �� Ϊ �� ��
        status = GXSetEnum(hDevice_, GX_ENUM_LINE_MODE, GX_ENUM_LINE_MODE_INPUT);

        //�� �� �� �� �� �� Դ Ϊ �� �� ��,�� �� �� �� ��
        //emStatus = GXSetEnum(hDevice_, GX_ENUM_LINE_SOURCE, GX_ENUM_LINE_SOURCE_STROBE);
        status = GXSetEnum(hDevice_ ,GX_ENUM_TRIGGER_ACTIVATION, GX_TRIGGER_ACTIVATION_RISINGEDGE);
        status = GXSetEnum(hDevice_ ,GX_ENUM_TRIGGER_SOURCE, GX_TRIGGER_SOURCE_LINE2);

        status = GXSetAcqusitionBufferNumber(hDevice_, 72);
        camera_opened_state_ = true;
 
        status = GXGetInt(hDevice_, GX_INT_WIDTH, &image_width_); 
        status = GXGetInt(hDevice_, GX_INT_HEIGHT, &image_height_); 

 
    }
 
 
    return true;
}
bool CameraGalaxy::closeCamera()
{
    if (!camera_opened_state_)
    {
        return false;
    }

    GX_STATUS status = GX_STATUS_SUCCESS;
    status = GXCloseDevice(hDevice_);
    status = GXCloseLib();

    camera_opened_state_ = false;

    return true;
}
bool CameraGalaxy::switchToInternalTriggerMode()
{
    GX_STATUS status;
    status = GXSetEnum(hDevice_, GX_ENUM_LINE_SOURCE, GX_ENUM_LINE_SOURCE_STROBE);
    

    if(GX_STATUS_SUCCESS != status)
    {
        return false;
    }

    status = GXSetEnum(hDevice_, GX_ENUM_TRIGGER_MODE, GX_TRIGGER_MODE_OFF);
    
    if(GX_STATUS_SUCCESS != status)
    {
        return false;
    }
      
    return true;
}
bool CameraGalaxy::switchToExternalTriggerMode()
{
    GX_STATUS status;
    status = GXSetEnum(hDevice_ ,GX_ENUM_TRIGGER_SOURCE, GX_TRIGGER_SOURCE_LINE2);

    if(GX_STATUS_SUCCESS != status)
    {
        return false;
    }
      
    return true;
    
}

bool CameraGalaxy::getExposure(double &val)
{
    GX_STATUS status = GX_STATUS_SUCCESS;  
    status = GXGetFloat(hDevice_, GX_FLOAT_EXPOSURE_TIME, &val);

    if (status != GX_STATUS_SUCCESS)
    {

        LOG(INFO) << "Error Status: " << status;
        return false;
    }
 

    return true;
}
bool CameraGalaxy::setExposure(double val)
{
    GX_STATUS status = GX_STATUS_SUCCESS; 
    status = GXSetFloat(hDevice_, GX_FLOAT_EXPOSURE_TIME, val);


    if(status != GX_STATUS_SUCCESS)
    {
        LOG(INFO)<<"Error Status: "<<status;
        return false;
    }
 
	return true;
}

bool CameraGalaxy::getGain(double &val)
{
    GX_STATUS status = GX_STATUS_SUCCESS;  
    status = GXGetFloat(hDevice_, GX_FLOAT_GAIN, &val); 

    if(status != GX_STATUS_SUCCESS)
    { 
        
        LOG(INFO)<<"Error Status: "<<status;
        return false;
    } 
 
	return true; 
}
bool CameraGalaxy::setGain(double val)
{
    GX_STATUS status = GX_STATUS_SUCCESS; 
    status = GXSetFloat(hDevice_, GX_FLOAT_GAIN, val); 

    if(status != GX_STATUS_SUCCESS)
    {
        
        LOG(INFO)<<"Error Status: "<<status;
        return false;
    }
 
	return true;
} 