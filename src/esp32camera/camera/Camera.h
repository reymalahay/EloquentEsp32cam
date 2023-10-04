#ifndef ELOQUENT_ESP32CAMERA_CAMERA_CAMERA
#define ELOQUENT_ESP32CAMERA_CAMERA_CAMERA

#include <esp_camera.h>
#include "./JpegQuality.h"
#include "./Brownout.h"
#include "./XCLK.h"
#include "./Resolution.h"
#include "./Pinout.h"
#include "./Sensor.h"
#include "./Converter565.h"
#include "../extra/exception.h"
#include "../extra/time/RateLimit.h"

using Eloquent::Extra::Exception;
using Eloquent::Extra::Time::RateLimit;

namespace Eloquent {
    namespace Esp32cam {
        namespace Camera {
            /**
             * Configure and use the camera,
             * Eloquent style
             */
            class Camera {
                public:
                    camera_config_t config;
                    camera_fb_t *frame;
                    JpegQuality quality;
                    Brownout brownout;
                    XCLK xclk;
                    Resolution resolution;
                    Pinout pinout;
                    Sensor sensor;
                    Exception exception;
                    RateLimit rateLimit;
                    Converter565<Camera> rgb565;

                    /**
                     * Constructor
                     */
                    Camera() :
                        exception("Camera"),
                        rgb565(this) {

                    }

                    /**
                     *
                     * @return
                     */
                    Exception& begin() {
                        // assert pinout is set
                        if (!pinout)
                            return exception.set("Pinout not set");

                        // assign pins
                        config.pin_d0 = pinout.pins.d0;
                        config.pin_d1 = pinout.pins.d1;
                        config.pin_d2 = pinout.pins.d2;
                        config.pin_d3 = pinout.pins.d3;
                        config.pin_d4 = pinout.pins.d4;
                        config.pin_d5 = pinout.pins.d5;
                        config.pin_d6 = pinout.pins.d6;
                        config.pin_d7 = pinout.pins.d7;
                        config.pin_xclk = pinout.pins.xclk;
                        config.pin_pclk = pinout.pins.pclk;
                        config.pin_vsync = pinout.pins.vsync;
                        config.pin_href = pinout.pins.href;
                        config.pin_sccb_sda = pinout.pins.sccb_sda;
                        config.pin_sccb_scl = pinout.pins.sccb_scl;
                        config.pin_pwdn = pinout.pins.pwdn;
                        config.pin_reset = pinout.pins.reset;

                        config.ledc_channel = LEDC_CHANNEL_0;
                        config.ledc_timer = LEDC_TIMER_0;
                        config.fb_count = 1;
                        config.pixel_format = PIXFORMAT_JPEG;
                        config.frame_size = resolution.framesize;
                        config.jpeg_quality = quality.quality;
                        config.xclk_freq_hz = xclk.freq;

                        // init
                        if (esp_camera_init(&config) != ESP_OK)
                            return exception.set("Cannot init camera");

                        sensor.setFrameSize(resolution.framesize);

                        return exception.clear();
                    }

                    /**
                     * Capture new frame
                     */
                    Exception& capture() {
                        if (!rateLimit)
                            return exception.set("Too many requests for frame");

                        free();
                        frame = esp_camera_fb_get();
                        rateLimit.touch();

                        if (!hasFrame())
                            return exception.set("Cannot capture frame");

                        return exception.clear();
                    }

                    /**
                     * Release frame memory
                     */
                    void free() {
                        if (frame != NULL) {
                            esp_camera_fb_return(frame);
                            frame = NULL;
                        }
                    }

                    /**
                     * Test if camera has a valid frame
                     */
                    inline bool hasFrame() const {
                        return frame != NULL && frame->len > 0;
                    }

                    /**
                     * Get frame size in bytes
                     * @return
                     */
                    inline size_t getSizeInBytes() {
                        return hasFrame() ? frame->len : 0;
                    }

                    /**
                     * Save to given folder with automatic name
                     */
                    template<typename Disk>
                    Exception& saveTo(Disk& disk, String folder = "") {
                        return saveToAs(disk, folder, "");
                    }

                    /**
                     * Save to given folder with given name
                     */
                    template<typename Disk>
                    Exception& saveToAs(Disk& disk, String folder = "", String filename = "") {
                        if (!hasFrame())
                            return exception.set("No frame to save");

                        if (filename == "")
                            filename = disk.getNextFilename("jpg");

                        if (folder != "")
                            filename = folder + '/' + filename;

                        if (filename[0] != '/')
                            filename = String('/') + filename;
                        
                        return disk.writeBinary(filename, frame->buf, frame->len);
                    }

                protected:
            };
        }
    }
}

namespace e {
    static Eloquent::Esp32cam::Camera::Camera camera;
}

#endif