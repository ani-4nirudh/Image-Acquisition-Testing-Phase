#include <iostream>
#include <filesystem>
#include <opencv4/opencv2/opencv.hpp>
#include "xlsxwriter.h"
#include "VmbCPP/VmbCPP.h"

using namespace VmbCPP;

/*
This function creates folders.

func: create_folders()
param: relative path with respect to build folder
return: void
*/
void create_folders(const std::string &path);

int main()
{
    VmbSystem &system = VmbSystem::GetInstance();

    VmbErrorType err;
    FeaturePtr feature;
    CameraPtrVector cameras;
    StreamPtrVector streams;
    FramePtr frame;
    VmbUint32_t frame_h, frame_w;
    VmbUint32_t t_out = 50; // timeout in miliseconds to allow frame to be filled
    VmbUint64_t time_stamp;
    VmbUchar_t *pImage = nullptr;

    // Press enter to close OpenCV window
    const int ENTER_KEY_CODE = 13;

    // Check whether API starts or not
    err = system.Startup();
    if (err != VmbErrorSuccess)
    {
        std::cout << "Could not start the API." << std::endl;
        return EXIT_FAILURE;
    }

    // Get a list of connected cameras
    err = system.GetCameras(cameras);
    if (err != VmbErrorSuccess)
    {
        std::cout << "No cameras found." << std::endl;
        system.Shutdown();
        return EXIT_FAILURE;
    }

    // Allow full access to the camera
    err = cameras[0]->Open(VmbAccessModeFull);
    if (err != VmbErrorSuccess)
    {
        std::cout << "Cannot access the cameras." << std::endl;
        return EXIT_FAILURE;
    }

    // Allow streaming from camera
    err = cameras[0]->GetStreams(streams);
    if (err != VmbErrorSuccess)
    {
        std::cout << "Not able to stream." << std::endl;
        return EXIT_FAILURE;
    }

    /**************************************** START -> Get & Set the camera parameters **************************************/
    double exposure_time, gain, black_lvl, current_fps, max_fps;

    std::cout << "\n///////////////////////////////\n"
              << "//// Printing general info ////\n"
              << "///////////////////////////////\n" <<std::endl;

    // Get Exposure Time
    cameras[0]->GetFeatureByName("ExposureTimeAbs", feature);
    feature->GetValue(exposure_time);
    std::cout << "/// Exposure Time (Before)     :        " << exposure_time << " us" << std::endl;

    // Set Exposure Time
    exposure_time = 150.0; // in microseconds
    cameras[0]->GetFeatureByName("ExposureTimeAbs", feature);
    feature->SetValue(exposure_time);
    std::cout << "/// Exposure Time (After)      :        " << exposure_time << " us" << std::endl;

    // Get Gain
    cameras[0]->GetFeatureByName("Gain", feature);
    feature->GetValue(gain);
    std::cout << "/// Gain (Before)              :        " << gain << std::endl;

    // Set Gain
    gain = 0.0;
    cameras[0]->GetFeatureByName("Gain", feature);
    feature->SetValue(gain);
    std::cout << "/// Gain (After)               :        " << gain << std::endl;

    // Get Black Level
    cameras[0]->GetFeatureByName("BlackLevel", feature);
    feature->GetValue(black_lvl);
    std::cout << "/// Black Level                :        " << black_lvl << std::endl;

    // Get Current Frame Rate
    cameras[0]->GetFeatureByName("AcquisitionFrameRateAbs", feature);
    feature->GetValue(current_fps);
    std::cout << "/// Frame Rate (Before)        :        " << current_fps << " fps" << std::endl;

    // Set Current Frame Rate
    current_fps = 200;
    cameras[0]->GetFeatureByName("AcquisitionFrameRateAbs", feature);
    feature->SetValue(max_fps);
    std::cout << "/// Frame Rate (After)         :        " << current_fps << " fps" << std::endl;

    // Get Max. Possible Frame Rate
    cameras[0]->GetFeatureByName("AcquisitionFrameRateLimit", feature);
    feature->GetValue(max_fps);
    std::cout << "/// Max. Possible Frame Rate   :        " << max_fps << " fps" << std::endl;


    std::cout << "\n///////////////////////////////\n"
              << "///////////// Done ////////////\n"
              << "///////////////////////////////\n" << std::endl;

    /**************************************** END -> Get & Set the camera parameters *******************************************/

    std::string root_folder = "../images";
    std::string param_folder_name = "Gain_" + std::to_string(int(gain)) + "_ExposureTime_" + std::to_string(int(exposure_time));
    std::string movement_folder_name = "X03_Y03_TopRight";
    // std::string movement_folder_name = "Calib_Y01_new_mount";
    std::string experiment_folder_name = "LaserDia_9mm";
    std::string testing_folder = root_folder + "/" + param_folder_name + "/" + movement_folder_name + "/" + experiment_folder_name;
    std::string timestamps_folder = "../timestamps/" + param_folder_name + "/" + movement_folder_name + "/" + experiment_folder_name;
    
    // Create folders in order to save images
    create_folders(testing_folder);

    // Create folders to save timestamps of the captured images
    create_folders(timestamps_folder);

    // Path to excel sheet
    std::string xlsx_path = timestamps_folder + "/" + "timestamps.xlsx";
    lxw_workbook *workbook = workbook_new(xlsx_path.c_str());
    lxw_worksheet *worksheet = workbook_add_worksheet(workbook, "Timestamps");
    worksheet_write_string(worksheet, 0, 0, "Timestamps (ns)", NULL);

    int frame_count = 0;
    std::string file_path;

    // Saving Images
    while (true)
    {
        // Acquire single frame
        err = cameras[0]->AcquireSingleImage(frame, t_out);
        if (err == VmbErrorSuccess)
        {
            // Get height of frame
            err = frame->GetHeight(frame_h);
            if (err != VmbErrorSuccess)
            {
                std::cout << "Failed to get frame height!\n" <<std::endl;
            }

            // Get width of frame
            err = frame->GetWidth(frame_w);
            if (err != VmbErrorSuccess)
            {
                std::cout << "Failed to get frame width!\n" <<std::endl;
            }

            // Get the image data
            err = frame->GetImage(pImage);
            if (err != VmbErrorSuccess)
            {
                std::cout << "Failed to acquire image data. \n" << std::endl;
            }

            // Get the timestamp of the acquired image
            err = frame->GetTimestamp(time_stamp);
            if (err != VmbErrorSuccess)
            {
                std::cout << "Failed to acquire timestamp. \n" << std::endl;
            }

            // Convert image data to OpenCV format
            cv::Mat cvMat(frame_h, frame_w, CV_8UC1, pImage);

            // Save the image
            file_path = testing_folder + "/frame_" + std::to_string(frame_count) + ".png";
            cv::imwrite(file_path, cvMat);
            cv::imshow("Frame Window (Press 'Enter' to quit)", cvMat);

            worksheet_write_number(worksheet, (frame_count + 1), 0, time_stamp, NULL);
            frame_count++;

            // Press enter to exit the program
            if (cv::waitKey(1) == ENTER_KEY_CODE)
            {
                break;
            }
        }
    }
    cv::destroyAllWindows();
    cameras[0]->Close();
    system.Shutdown();
    workbook_close(workbook);

    return EXIT_SUCCESS;
}

void create_folders(const std::string &path)
{
    if (!std::filesystem::exists(path))
    {
        std::cout << "\n///////////////////////////////////////////////////////////////////////\n"
                  << "/// Creating folder at        :       " << path
                  << "\n///////////////////////////////////////////////////////////////////////\n"
                  << std::endl;
        std::filesystem::create_directories(path);
    } else
    {
        std::cout << "\n///////////////////////////////////////////////////////////////////////\n"
                  << "/// Folder exists at          :       " << path
                  << "\n///////////////////////////////////////////////////////////////////////\n"
                  << std::endl;
    }
}