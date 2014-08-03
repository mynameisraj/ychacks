//
//  pinky-test.cpp
//  ychacks
//
//  Created by Raj Ramamurthy on 8/2/14.
//  Copyright (c) 2014 Raj Ramamurthy. All rights reserved.
//

#include <myo/myo.hpp>
#include <stdio.h>
#include <math.h>
#include <dispatch/dispatch.h>
#include "portaudio.h"

#define NUM_SECONDS   (0.2)
#define SAMPLE_RATE   (44100)

static int patestCallback( const void *inputBuffer, void *outputBuffer,
                          unsigned long framesPerBuffer,
                          const PaStreamCallbackTimeInfo* timeInfo,
                          PaStreamCallbackFlags statusFlags,
                          void *userData );
int play_sound(long);

class DataCollector : public myo::DeviceListener {
public:
    DataCollector()
    : onArm(false), currentPose()
    {
    }

    void onPose(myo::Myo* myo, uint64_t timestamp, myo::Pose pose)
    {
        currentPose = pose;
    }

    void onArmRecognized(myo::Myo* myo, uint64_t timestamp, myo::Arm arm, myo::XDirection xDirection)
    {
        onArm = true;
        whichArm = arm;
    }

    void onArmLost(myo::Myo* myo, uint64_t timestamp)
    {
        onArm = false;
    }

    void print()
    {
        std::cout << '\r';

        if (onArm) {
            std::string poseString = currentPose.toString();
            std::cout << '[' << (whichArm == myo::armLeft ? "L" : "R") << ']'
                      << '[' << poseString << std::string(14 - poseString.size(), ' ') << ']';
        } else {
            std::cout << "[?]" << '[' << std::string(14, ' ') << ']';
        }

        if (currentPose == myo::Pose::fist) {
            play_sound(SAMPLE_RATE);
        }

        std::cout << std::flush;
    }

    // These values are set by the methods above
    bool onArm;
    myo::Arm whichArm;
    myo::Pose currentPose;
};

int main(int argc, char** argv)
{
    try {

    myo::Hub hub("com.ychacks.test");
    std::cout << "Attempting to find a Myo..." << std::endl;
    myo::Myo* myo = hub.waitForMyo(10000);

    if (!myo) {
        throw std::runtime_error("Unable to find a Myo!");
    }

    std::cout << "Connected to a Myo armband!" << std::endl << std::endl;
    DataCollector collector;
    hub.addListener(&collector);

    while (1) {
        // In this case, we wish to update our display 20 times a second, so we run for 1000/20 milliseconds.
        hub.run(1000/20);
        collector.print();
    }

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        std::cerr << "Press enter to continue.";
        std::cin.ignore();
        return 1;
    }
}

#pragma mark Portaudio



typedef struct
{
    float left_phase;
    float right_phase;
}
paTestData;

/* This routine will be called by the PortAudio engine when audio is needed.
 ** It may called at interrupt level on some machines so don't do anything
 ** that could mess up the system like calling malloc() or free().
 */
static int patestCallback( const void *inputBuffer, void *outputBuffer,
                          unsigned long framesPerBuffer,
                          const PaStreamCallbackTimeInfo* timeInfo,
                          PaStreamCallbackFlags statusFlags,
                          void *userData )
{
    /* Cast data passed through stream to our structure. */
    paTestData *data = (paTestData*)userData;
    float *out = (float*)outputBuffer;
    unsigned int i;
    (void) inputBuffer; /* Prevent unused variable warning. */

    for( i=0; i<framesPerBuffer; i++ )
    {
        *out++ = data->left_phase;  /* left */
        *out++ = data->right_phase;  /* right */
        /* Generate simple sawtooth phaser that ranges between -1.0 and 1.0. */
        data->left_phase += 0.01f;
        /* When signal reaches top, drop back down. */
        if( data->left_phase >= 1.0f ) data->left_phase -= 2.0f;
        /* higher pitch so we can distinguish left and right. */
        data->right_phase += 0.05f;
        if( data->right_phase >= 1.0f ) data->right_phase -= 2.0f;
    }
    return 0;
}

/*******************************************************************/
static paTestData data;
int play_sound(long sample_rate)
{
    PaStream *stream;
    PaError err;

    printf("PortAudio Test: output sawtooth wave.\n");
    /* Initialize our data for use by callback. */
    data.left_phase = data.right_phase = 0.0;
    /* Initialize library before making any other calls. */
    err = Pa_Initialize();
    if( err != paNoError ) goto error;

    /* Open an audio I/O stream. */
    err = Pa_OpenDefaultStream( &stream,
                               0,          /* no input channels */
                               2,          /* stereo output */
                               paFloat32,  /* 32 bit floating point output */
                               sample_rate,
                               256,        /* frames per buffer */
                               patestCallback,
                               &data );
    if( err != paNoError ) goto error;

    err = Pa_StartStream( stream );
    if( err != paNoError ) goto error;

    /* Sleep for several seconds. */
    Pa_Sleep(NUM_SECONDS*1000);

    err = Pa_StopStream( stream );
    if( err != paNoError ) goto error;
    err = Pa_CloseStream( stream );
    if( err != paNoError ) goto error;
    Pa_Terminate();
    printf("Test finished.\n");
    return err;
error:
    Pa_Terminate();
    fprintf( stderr, "An error occured while using the portaudio stream\n" );
    fprintf( stderr, "Error number: %d\n", err );
    fprintf( stderr, "Error message: %s\n", Pa_GetErrorText( err ) );
    return err;
}
