/*
 * Sources:
 *  https://www.letscontrolit.com/wiki/index.php?title=GP2Y10#Calibrating_approximately
 *  https://github.com/sharpsensoruser/sharp-sensor-demos/blob/master/sharp_gp2y1014au0f_demo/sharp_gp2y1014au0f_demo.ino
 *  http://www.howmuchsnow.com/arduino/airquality/
 *  https://wiki.keyestudio.com/Ks0196_keyestudio_PM2.5_Shield
 */
#include "esphome.h"

#define GPIO32 = 32;
#define GPIO34 = 34;

// Arduino pin numbers.
const int sharpLEDPin = 32; // Arduino digital pin 7 connect to sensor LED.
const int sharpVoPin = 34;  // Arduino analog pin 5 connect to sensor Vo.

const int SAMPLE_SIZE = 100;

class My_dust_Sensor : public Component
{
private:
    // Set the typical output voltage in Volts when there is zero dust.
    float Voc;

    // Use the typical sensitivity in units of V per 100ug/m3.
    const float K = 0.5;

    unsigned long VoRawTotal = 0;
    int VoRawCount = 0;

    int ledOutputPin = 32;
    int analogReadPin = 34;

public:
    Sensor *dust_density_sensor = new Sensor();
    Sensor *dust_alt_density_sensor = new Sensor();
    Sensor *dust_sensor = new Sensor();
    Sensor *dust_ppm = new Sensor();

    My_dust_Sensor() : Component()
    {
        Voc = 0.6;
    }

    /**
     * Raw sensor reading from ADC
     * 
     * @return uint16_t value between 0 - 1024
     */
    uint16_t readDustRawOnce()
    {
        // Turn on the dust sensor LED by setting digital pin LOW.
        digitalWrite(this->ledOutputPin, LOW);

        // Wait 0.28ms before taking a reading of the output voltage as per spec.
        delayMicroseconds(280);

        // Record the output voltage. This operation takes around 100 microseconds.
        uint16_t VoRaw = analogRead(this->analogReadPin);

        // Turn the dust sensor LED off by setting digital pin HIGH.
        digitalWrite(this->ledOutputPin, HIGH);

        // Wait for remainder of the 10ms cycle = 10000 - 280 - 100 microseconds.
        delayMicroseconds(9680); // delay

        return VoRaw;
    }

    void setup() override
    {
        pinMode(this->ledOutputPin, OUTPUT); // sensor led pin
        pinMode(this->analogReadPin, INPUT); // output form sensor
    }

    void loop() override
    {
        // // Power on the LED
        // digitalWrite(32, LOW);

        // delayMicroseconds(280);

        // int VoRaw = analogRead(sharpVoPin);

        // digitalWrite(32, HIGH); // turn the LED off

        // // Wait for remainder of the 10ms cycle = 10000 - 280 - 100 microseconds.
        // delayMicroseconds(9680); // delay

        float VoRaw = this->readDustRawOnce();
        float Vo = VoRaw;

        VoRawTotal += VoRaw;
        VoRawCount++;
        if (VoRawCount >= SAMPLE_SIZE)
        {
            Vo = 1.0 * VoRawTotal / SAMPLE_SIZE;
            VoRawCount = 0;
            VoRawTotal = 0;
        }
        else
        {
            return;
        }

        Vo = Vo / 1024.0 * 5.0;
        ESP_LOGI("custom", "Sensor voltage %fmV:", Vo * 1000.0);

        // Convert to Dust Density in units of ug/m3.
        float dV = Vo - Voc;
        if (dV < 0)
        {
            dV = 0;
            Voc = Vo;
        }
        float dustDensity = dV / K * 100.0;

        ESP_LOGI("custom", "DustDensity %f ug/m3", dustDensity);
        dust_density_sensor->publish_state(dustDensity);

        float ppmpercf = 0;

        ppmpercf = (Vo - 0.0256) * 120000;
        if (ppmpercf < 0)
        {
            ppmpercf = 0;
        }
        dust_ppm->publish_state(ppmpercf);

        // 0 - 5V mapped to 0 - 1023 integer values
        // linear eqaution taken from http://www.howmuchsnow.com/arduino/airquality/
        // Chris Nafis (c) 2012
        dustDensity = 0.17 * Vo - 0.1;
        if (dustDensity < 0)
        {
            dustDensity = 0;
        }
        if (dustDensity > 0.5)
        {
            dustDensity = 0.5;
        }

        dustDensity = dustDensity * 1000;
        dust_alt_density_sensor->publish_state(dustDensity);

        if (Vo > 36.455)
        {
            float dust_value = (float(Vo / 1024) - 0.0356) * 120000 * 0.035;
            dust_sensor->publish_state(dust_value);
        }
    }
};