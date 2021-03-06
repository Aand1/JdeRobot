//
// Created by frivas on 4/04/17.
//

#include <visionlib/colorspaces/imagecv.h>
#include <image.h>
#include <zlib.h>
#include "CameraUtils.h"
#include <logger/Logger.h>

cv::Mat CameraUtils::getImageFromCameraProxy(jderobot::ImageDataPtr dataPtr) {
    cv::Mat outImage;
    colorspaces::Image::FormatPtr fmt;

    fmt = colorspaces::Image::Format::searchFormat(dataPtr->description->format);
    if (!fmt)
        throw "Format not supported";

    if (dataPtr->description->format == colorspaces::ImageRGB8::FORMAT_RGB8_Z.get()->name ||
        dataPtr->description->format == colorspaces::ImageRGB8::FORMAT_DEPTH8_16_Z.get()->name	)
    {

        size_t dest_len = dataPtr->description->width*dataPtr->description->height*3;
        size_t source_len = dataPtr->pixelData.size();

        unsigned char* origin_buf = (uchar*) malloc(dest_len);

        int r = uncompress((Bytef *) origin_buf, (uLongf *) &dest_len, (const Bytef *) &(dataPtr->pixelData[0]), (uLong)source_len);

        if(r != Z_OK) {
            fprintf(stderr, "[CMPR] Error:\n");
            switch(r) {
                case Z_MEM_ERROR:
                    fprintf(stderr, "[CMPR] Error: Not enough memory to compress.\n");
                    break;
                case Z_BUF_ERROR:
                    fprintf(stderr, "[CMPR] Error: Target buffer too small.\n");
                    break;
                case Z_STREAM_ERROR:    // Invalid compression level
                    fprintf(stderr, "[CMPR] Error: Invalid compression level.\n");
                    break;
            }
        }
        else
        {
            colorspaces::Image imageRGB(dataPtr->description->width,dataPtr->description->height,colorspaces::ImageRGB8::FORMAT_RGB8,&(origin_buf[0]));
            colorspaces::ImageRGB8 img_rgb888(imageRGB);//conversion will happen if needed
            cv::Mat(cvSize(img_rgb888.width,img_rgb888.height), CV_8UC3, img_rgb888.data).copyTo(outImage);
            img_rgb888.release();
        }


        if (origin_buf)
            free(origin_buf);

    }
    else if (dataPtr->description->format == colorspaces::ImageRGB8::FORMAT_RGB8.get()->name ||
             dataPtr->description->format == colorspaces::ImageRGB8::FORMAT_DEPTH8_16.get()->name  )
    {
        colorspaces::Image imageRGB(dataPtr->description->width,dataPtr->description->height,colorspaces::ImageRGB8::FORMAT_RGB8,&(dataPtr->pixelData[0]));
        colorspaces::ImageRGB8 img_rgb888(imageRGB);//conversion will happen if needed
        cv::Mat(cvSize(img_rgb888.width,img_rgb888.height), CV_8UC3, img_rgb888.data).copyTo(outImage);
        img_rgb888.release();
    }
    else if (dataPtr->description->format == colorspaces::ImageGRAY8::FORMAT_GRAY8_Z.get()->name) {
        //gay compressed
        size_t dest_len = dataPtr->description->width*dataPtr->description->height;
        size_t source_len = dataPtr->pixelData.size();

        unsigned char* origin_buf = (uchar*) malloc(dest_len);

        int r = uncompress((Bytef *) origin_buf, (uLongf *) &dest_len, (const Bytef *) &(dataPtr->pixelData[0]), (uLong)source_len);

        if(r != Z_OK) {
            fprintf(stderr, "[CMPR] Error:\n");
            switch(r) {
                case Z_MEM_ERROR:
                    fprintf(stderr, "[CMPR] Error: Not enough memory to compress.\n");
                    break;
                case Z_BUF_ERROR:
                    fprintf(stderr, "[CMPR] Error: Target buffer too small.\n");
                    break;
                case Z_STREAM_ERROR:    // Invalid compression level
                    fprintf(stderr, "[CMPR] Error: Invalid compression level.\n");
                    break;
            }
        }
        else
        {
            colorspaces::Image imageGray(dataPtr->description->width,dataPtr->description->height,colorspaces::ImageGRAY8::FORMAT_GRAY8,&(origin_buf[0]));
            colorspaces::ImageGRAY8 img_gray8(imageGray);//conversion will happen if needed

            cv::Mat(cvSize(img_gray8.width,img_gray8.height), CV_8UC1, img_gray8.data).copyTo(outImage);
            img_gray8.release();
        }


        if (origin_buf)
            free(origin_buf);
    }
    else if (dataPtr->description->format == colorspaces::ImageGRAY8::FORMAT_GRAY8.get()->name){
        colorspaces::Image imageGray(dataPtr->description->width,dataPtr->description->height,colorspaces::ImageGRAY8::FORMAT_GRAY8,&(dataPtr->pixelData[0]));
        colorspaces::ImageGRAY8 img_gray8(imageGray);//conversion will happen if needed
        cv::Mat(cvSize(img_gray8.width,img_gray8.height), CV_8UC1, img_gray8.data).copyTo(outImage);
        img_gray8.release();
    }
    else{
        LOG(ERROR) << "Unkown image format";
    }

    return outImage;
}

std::string CameraUtils::negotiateDefaultFormat(jderobot::CameraPrx prx, const std::string& definedFormat) {

    std::string format;
    // Discover what format are supported.
    jderobot::ImageFormat formats = prx->getImageFormat();

    std::vector<std::string>::iterator it;
    it = std::find(formats.begin(), formats.end(), definedFormat);
    if (it==formats.end()){
        it = std::find(formats.begin(), formats.end(), colorspaces::ImageRGB8::FORMAT_RGB8.get()->name);

        if (it != formats.end())
        {
            format = colorspaces::ImageRGB8::FORMAT_RGB8.get()->name;
            it = std::find(formats.begin(), formats.end(), colorspaces::ImageRGB8::FORMAT_RGB8_Z.get()->name);
            if (it != formats.end())
                format = colorspaces::ImageRGB8::FORMAT_RGB8_Z.get()->name;
        }
        else
        {
            it = std::find(formats.begin(), formats.end(), colorspaces::ImageRGB8::FORMAT_DEPTH8_16.get()->name);
            if (it != formats.end())
            {
                format = colorspaces::ImageRGB8::FORMAT_DEPTH8_16.get()->name;
                it = std::find(formats.begin(), formats.end(), colorspaces::ImageRGB8::FORMAT_DEPTH8_16_Z.get()->name);
                if (it != formats.end())
                    format = colorspaces::ImageRGB8::FORMAT_DEPTH8_16_Z.get()->name;
            }
            else{
                format = colorspaces::ImageGRAY8::FORMAT_GRAY8.get()->name;
                it = std::find(formats.begin(), formats.end(), colorspaces::ImageGRAY8::FORMAT_GRAY8_Z.get()->name);
                if (it != formats.end())
                    format = colorspaces::ImageGRAY8::FORMAT_GRAY8_Z.get()->name;
            }
        }
    }
    else{
        format = definedFormat;
    }
    LOG(INFO) << "Negotiated format " + format + " for camera " + prx->getCameraDescription()->name;
    return format;
}
