/*
 * apds9960.c
 *
 *  Created on: 2018年5月12日
 *      Author: guo
 */

#include "apds9960.h"

int gesture_ud_delta_=0;
int gesture_lr_delta_=0;
int gesture_ud_count_=0;
int gesture_lr_count_=0;
int gesture_near_count_=0;
int gesture_far_count_=0;
int gesture_state_=0;
int gesture_motion_=0;

gesture_data_type gesture_data_;
#define FIFO_PAUSE_TIME 30

bool APDS9960_Init(void)
{
    uint8_t id;

    /* Read ID register and check against known values for APDS-9960 */
    IIC_ReadData(APDS9960_I2C_ADDR,APDS9960_ID,&id);
    if(id!=APDS9960_DEVICE_ID)
        return false;

    /* Set ENABLE register to 0 (disable all features) */
    if(APDS9960_setMode(APDS9960_MODE_ALL, false)==false)
        return false;

    /* Set default values for ambient light and proximity registers */
    IIC_WriteReg(APDS9960_I2C_ADDR, APDS9960_ATIME, DEFAULT_ATIME);
    IIC_WriteReg(APDS9960_I2C_ADDR, APDS9960_WTIME, DEFAULT_WTIME);
    IIC_WriteReg(APDS9960_I2C_ADDR, APDS9960_PPULSE, DEFAULT_PROX_PPULSE);
    IIC_WriteReg(APDS9960_I2C_ADDR, APDS9960_POFFSET_UR, DEFAULT_POFFSET_UR);
    IIC_WriteReg(APDS9960_I2C_ADDR, APDS9960_POFFSET_DL, DEFAULT_POFFSET_DL);
    IIC_WriteReg(APDS9960_I2C_ADDR, APDS9960_CONFIG1, DEFAULT_CONFIG1);
    APDS9960_SetLEDDrive(DEFAULT_LDRIVE);
    APDS9960_SetProximityGain(DEFAULT_PGAIN);
    APDS9960_SetAmbientLightGain(DEFAULT_AGAIN);
    APDS9960_SetProxIntLowThresh(DEFAULT_PILT);
    APDS9960_SetProxIntHighThresh(DEFAULT_PIHT);
    APDS9960_SetLightIntLowThreshold(DEFAULT_AILT);
    APDS9960_SetLightIntHighThreshold(DEFAULT_AIHT);
    IIC_WriteReg(APDS9960_I2C_ADDR, APDS9960_PERS, DEFAULT_PERS);
    IIC_WriteReg(APDS9960_I2C_ADDR, APDS9960_CONFIG2, DEFAULT_CONFIG2);
    IIC_WriteReg(APDS9960_I2C_ADDR, APDS9960_CONFIG3, DEFAULT_CONFIG3);

    /* Set default values for gesture sense registers */
    APDS9960_SetGestureEnterThresh(DEFAULT_GPENTH);
    APDS9960_SetGestureExitThresh(DEFAULT_GEXTH);
    IIC_WriteReg(APDS9960_I2C_ADDR, APDS9960_GCONF1, DEFAULT_GCONF1);

    APDS9960_SetGestureGain(DEFAULT_GGAIN);
    APDS9960_SetGestureLEDDrive(DEFAULT_GLDRIVE);
    APDS9960_SetGestureWaitTime(DEFAULT_GWTIME);
    IIC_WriteReg(APDS9960_I2C_ADDR, APDS9960_GOFFSET_U, DEFAULT_GOFFSET);
    IIC_WriteReg(APDS9960_I2C_ADDR, APDS9960_GOFFSET_D, DEFAULT_GOFFSET);
    IIC_WriteReg(APDS9960_I2C_ADDR, APDS9960_GOFFSET_L, DEFAULT_GOFFSET);
    IIC_WriteReg(APDS9960_I2C_ADDR, APDS9960_GOFFSET_R, DEFAULT_GOFFSET);
    IIC_WriteReg(APDS9960_I2C_ADDR, APDS9960_GPULSE, DEFAULT_GPULSE);
    IIC_WriteReg(APDS9960_I2C_ADDR, APDS9960_GCONF3, DEFAULT_GCONF3);

    APDS9960_SetGestureIntEnable(DEFAULT_GIEN);
    return true;
}

/***************************************************************************************************************
*Starts the gesture recognition engine on the APDS-9960
****************************************************************************************************************/
void APDS9960_enableGestureSensor(bool interrupts)
{
    APDS9960_resetGestureParameters();
    IIC_WriteReg(APDS9960_I2C_ADDR, APDS9960_WTIME, 0xff);
    IIC_WriteReg(APDS9960_I2C_ADDR, APDS9960_PPULSE, DEFAULT_GESTURE_PPULSE);
    APDS9960_SetLEDBoost(LED_BOOST_300);
    if( interrupts )
        APDS9960_SetGestureIntEnable(1);
    else
        APDS9960_SetGestureIntEnable(0);
    APDS9960_SetGestureMode(1);
    APDS9960_setMode(APDS9960_MODE_POWER, 1);
    APDS9960_setMode(APDS9960_MODE_WAIT,1);
    APDS9960_setMode(APDS9960_MODE_PROXIMITY,1);
    APDS9960_setMode(APDS9960_MODE_GESTURE,1);
}

/***************************************************************************************************************
*Resets all the parameters in the gesture data member
****************************************************************************************************************/
void APDS9960_resetGestureParameters(void)
{
    gesture_data_.index = 0;
    gesture_data_.total_gestures = 0;

    gesture_ud_delta_ = 0;
    gesture_lr_delta_ = 0;

    gesture_ud_count_ = 0;
    gesture_lr_count_ = 0;

    gesture_near_count_ = 0;
    gesture_far_count_ = 0;

    gesture_state_ = 0;
    gesture_motion_ = DIR_NONE;
}

/***************************************************************************************************************
*public methods for controlling the APDS-9960
****************************************************************************************************************/
uint8_t getMode(void)
{
    uint8_t enable_value;
    /* Read current ENABLE register */
    IIC_ReadData(APDS9960_I2C_ADDR,APDS9960_ENABLE, &enable_value);
    return enable_value;
}

/***************************************************************************************************************
*Enables or disables a feature in the APDS-9960
****************************************************************************************************************/
bool APDS9960_setMode(int8_t mode, bool enable)
{
    uint8_t data;

    /* Read current ENABLE register */
    //IIC_ReadData(APDS9960_I2C_ADDR, APDS9960_ENABLE, &data, 1);
    data=getMode();

    /* get None */
    if( data == 0xff )
        return false;

    /* Change bit(s) in ENABLE register */
    enable = enable & 0x01;

    if( (mode >= 0) && (mode <= 6) )
    {
        if (enable)
            data |= (1 << mode);
        else
            data &= ~(1 << mode);
    }
    else if( mode == APDS9960_MODE_ALL )
    {
        if (enable)
            data = 0x7F;
        else
            data = 0x00;
    }

    /* Write value back to ENABLE register */
    IIC_WriteReg(APDS9960_I2C_ADDR, APDS9960_ENABLE, data);

    return true;
}

/***************************************************************************************************************
*Sets the LED drive strength for proximity and ALS
****************************************************************************************************************/
void APDS9960_SetLEDDrive(uint8_t drive)
{
    uint8_t data;

    /* Read value from CONTROL register */
    IIC_ReadData(APDS9960_I2C_ADDR,APDS9960_CONTROL, &data);

    /* Set bits in register to given value */
    drive &= 0x03;
    drive = drive << 6;
    data &= 0x3f;
    data |= drive;

    /* Write register value back into CONTROL register */
    IIC_WriteReg(APDS9960_I2C_ADDR, APDS9960_CONTROL, data);
}

/***************************************************************************************************************
*Sets the receiver gain for proximity detection
****************************************************************************************************************/
void APDS9960_SetProximityGain(uint8_t drive)
{
    uint8_t data;

    /* Read value from CONTROL register */
    IIC_ReadData(APDS9960_I2C_ADDR, APDS9960_CONTROL, &data);

    /* Set bits in register to given value */
    drive &= 0x03;
    drive = drive << 2;
    data &= 0xf3;
    data |= drive;

    /* Write register value back into CONTROL register */
    IIC_WriteReg(APDS9960_I2C_ADDR, APDS9960_CONTROL, data);
}

/***************************************************************************************************************
*Sets the receiver gain for the ambient light sensor (ALS)
****************************************************************************************************************/
void APDS9960_SetAmbientLightGain(uint8_t drive)
{
    uint8_t data;

    /* Read value from CONTROL register */
    IIC_ReadData(APDS9960_I2C_ADDR, APDS9960_CONTROL, &data);

    /* Set bits in register to given value */
    drive &= 0x03;
    data &= 0xfc;
    data |= drive;

    /* Write register value back into CONTROL register */
    IIC_WriteReg(APDS9960_I2C_ADDR, APDS9960_CONTROL, data);
}

/***************************************************************************************************************
*Sets the lower threshold for proximity detection
****************************************************************************************************************/
void APDS9960_SetProxIntLowThresh(uint8_t threshold)
{
    IIC_WriteReg(APDS9960_I2C_ADDR, APDS9960_PILT, threshold);
}

/***************************************************************************************************************
*Sets the high threshold for proximity detection
****************************************************************************************************************/
void APDS9960_SetProxIntHighThresh(uint8_t threshold)
{
    IIC_WriteReg(APDS9960_I2C_ADDR, APDS9960_PIHT, threshold);
}

/***************************************************************************************************************
*Sets the low threshold for ambient light interrupts
****************************************************************************************************************/
void APDS9960_SetLightIntLowThreshold(uint16_t threshold)
{
    uint8_t data_low;
    uint8_t data_high;

    /* Break 16-bit threshold into 2 8-bit values */
    data_low = threshold & 0x00FF;
    data_high = (threshold & 0xFF00) >> 8;

    /* Write low byte */
    IIC_WriteReg(APDS9960_I2C_ADDR, APDS9960_AILTL, data_low);
    IIC_WriteReg(APDS9960_I2C_ADDR, APDS9960_AILTH, data_high);
}

/***************************************************************************************************************
*Sets the high threshold for ambient light interrupts
****************************************************************************************************************/
void APDS9960_SetLightIntHighThreshold(uint16_t threshold)
{
    uint8_t data_low;
    uint8_t data_high;

    /* Break 16-bit threshold into 2 8-bit values */
    data_low = threshold & 0x00FF;
    data_high = (threshold & 0xFF00) >> 8;

    /* Write low byte */
    IIC_WriteReg(APDS9960_I2C_ADDR, APDS9960_AIHTL, data_low);
    IIC_WriteReg(APDS9960_I2C_ADDR, APDS9960_AIHTH, data_high);
}

/***************************************************************************************************************
*Sets the entry proximity threshold for gesture sensing
****************************************************************************************************************/
void APDS9960_SetGestureEnterThresh(uint8_t threshold)
{
    IIC_WriteReg(APDS9960_I2C_ADDR, APDS9960_GPENTH, threshold);
}

/***************************************************************************************************************
*Sets the exit proximity threshold for gesture sensing
****************************************************************************************************************/
void APDS9960_SetGestureExitThresh(uint8_t threshold)
{
    IIC_WriteReg(APDS9960_I2C_ADDR, APDS9960_GEXTH, threshold);
}

/***************************************************************************************************************
*Sets the gain of the photodiode during gesture mode
****************************************************************************************************************/
void APDS9960_SetGestureGain(uint8_t gain)
{
    uint8_t data;

    /* Read value from GCONF2 register */
    IIC_ReadData(APDS9960_I2C_ADDR, APDS9960_GCONF2, &data);

    /* Set bits in register to given value */
    gain &= 0x03;
    gain = gain << 5;
    data &= 0x9f;
    data |= gain;

    /* Write register value back into GCONF2 register */
    IIC_WriteReg(APDS9960_I2C_ADDR, APDS9960_GCONF2, data);
}

/***************************************************************************************************************
*Sets the LED drive current during gesture mode
****************************************************************************************************************/
void APDS9960_SetGestureLEDDrive(uint8_t drive)
{
    uint8_t data;

    /* Read value from GCONF2 register */
    IIC_ReadData(APDS9960_I2C_ADDR, APDS9960_GCONF2, &data);

    /* Set bits in register to given value */
    drive &= 0x03;
    drive = drive << 3;
    data &= 0xe7;
    data |= drive;

    /* Write register value back into GCONF2 register */
    IIC_WriteReg(APDS9960_I2C_ADDR, APDS9960_GCONF2, data);
}


/***************************************************************************************************************
*Sets the time in low power mode between gesture detections
****************************************************************************************************************/
void APDS9960_SetGestureWaitTime(uint8_t time)
{
    uint8_t data;

    /* Read value from GCONF2 register */
    IIC_ReadData(APDS9960_I2C_ADDR, APDS9960_GCONF2, &data);

    /* Set bits in register to given value */
    time &= 0x07;
    data &= 0xf8;
    data |= time;

    /* Write register value back into GCONF2 register */
    IIC_WriteReg(APDS9960_I2C_ADDR, APDS9960_GCONF2, data);
}

/***************************************************************************************************************
*Turns gesture-related interrupts on or off
****************************************************************************************************************/
void APDS9960_SetGestureIntEnable(uint8_t enable)
{
    uint8_t data;

    /* Read value from GCONF4 register */
    IIC_ReadData(APDS9960_I2C_ADDR, APDS9960_GCONF4, &data);

    /* Set bits in register to given value */
    enable &= 0x01;
    enable = enable << 1;
    data &= 0xfd;
    data |= enable;

    /* Write register value back into GCONF4 register */
    IIC_WriteReg(APDS9960_I2C_ADDR, APDS9960_GCONF4, data);
}

/***************************************************************************************************************
*Sets the LED current boost value
****************************************************************************************************************/
void APDS9960_SetLEDBoost(uint8_t boost)
{
    uint8_t data;

    /* Read value from CONFIG2 register */
    IIC_ReadData(APDS9960_I2C_ADDR, APDS9960_CONFIG2, &data);

    /* Set bits in register to given value */
    boost &= 0x03;
    boost = boost << 4;
    data &= 0xcf;
    data |= boost;

    /* Write register value back into CONFIG2 register */
    IIC_WriteReg(APDS9960_I2C_ADDR, APDS9960_CONFIG2, data);
}

/***************************************************************************************************************
*Tells the state machine to either enter or exit gesture state machine
****************************************************************************************************************/
void APDS9960_SetGestureMode(uint8_t mode)
{
    uint8_t data;

    /* Read value from GCONF4 register */
    IIC_ReadData(APDS9960_I2C_ADDR, APDS9960_GCONF4, &data);

    /* Set bits in register to given value */
    mode &= 0x01;
    data &= 0xfe;
    data |= mode;

    /* Write register value back into GCONF4 register */
    IIC_WriteReg(APDS9960_I2C_ADDR, APDS9960_GCONF4, data);
}

/***************************************************************************************************************
*Determines swipe direction or near/far state
****************************************************************************************************************/
bool APDS9960_decodeGesture(void)
{
    /* Return if near or far event is detected */
    if( gesture_state_ == NEAR_STATE )
    {
        gesture_motion_ = DIR_NEAR;
        return true;
    }
    else if ( gesture_state_ == FAR_STATE )
    {
        gesture_motion_ = DIR_FAR;
        return true;
    }

    /* Determine swipe direction */
    if((gesture_ud_count_ == -1) && (gesture_lr_count_ == 0))
        gesture_motion_ = DIR_RIGHT;
    else if((gesture_ud_count_ == 1) && (gesture_lr_count_ == 0))
        gesture_motion_ = DIR_LEFT;
    else if((gesture_ud_count_ == 0) && (gesture_lr_count_ == 1))
        gesture_motion_ = DIR_DOWN;
    else if( (gesture_ud_count_ == 0) && (gesture_lr_count_ == -1))
        gesture_motion_ = DIR_UP;
    else if( (gesture_ud_count_ == -1) && (gesture_lr_count_ == 1))
    {
        if(abs(gesture_ud_delta_) > abs(gesture_lr_delta_))
            gesture_motion_ = DIR_RIGHT;
        else
            gesture_motion_ = DIR_DOWN;
    }
    else if((gesture_ud_count_ == 1) && (gesture_lr_count_ == -1))
    {
        if(abs(gesture_ud_delta_) > abs(gesture_lr_delta_))
            gesture_motion_ = DIR_LEFT;
        else
            gesture_motion_ = DIR_UP;
    }
    else if( (gesture_ud_count_ == -1) && (gesture_lr_count_ == -1))
    {
        if(abs(gesture_ud_delta_) > abs(gesture_lr_delta_))
            gesture_motion_ = DIR_RIGHT;
        else
            gesture_motion_ = DIR_UP;
    }
    else if( (gesture_ud_count_ == 1) && (gesture_lr_count_ == 1))
    {
        if( abs(gesture_ud_delta_) > abs(gesture_lr_delta_))
            gesture_motion_ = DIR_LEFT;
        else
            gesture_motion_ = DIR_DOWN;
    }
    else
        return false;
    return true;
}

/***************************************************************************************************************
*Processes the raw gesture data to determine swipe direction
****************************************************************************************************************/
bool APDS9960_processGestureData(void)
{
    uint8_t u_first = 0;
    uint8_t d_first = 0;
    uint8_t l_first = 0;
    uint8_t r_first = 0;
    uint8_t u_last = 0;
    uint8_t d_last = 0;
    uint8_t l_last = 0;
    uint8_t r_last = 0;
    int ud_ratio_first;
    int lr_ratio_first;
    int ud_ratio_last;
    int lr_ratio_last;
    int ud_delta;
    int lr_delta;
    int i;

    /* If we have less than 4 total gestures, that's not enough */
    if( gesture_data_.total_gestures <= 4 )
    {
        return false;
    }

    /* Check to make sure our data isn't out of bounds */
    if( (gesture_data_.total_gestures <= 32) && (gesture_data_.total_gestures > 0) )
    {
        /* Find the first value in U/D/L/R above the threshold */
        for( i = 0; i < gesture_data_.total_gestures; i++ )
        {
            if((gesture_data_.u_data[i]>GESTURE_THRESHOLD_OUT)&&\
               (gesture_data_.d_data[i]>GESTURE_THRESHOLD_OUT)&&\
               (gesture_data_.l_data[i]>GESTURE_THRESHOLD_OUT)&&\
               (gesture_data_.r_data[i]>GESTURE_THRESHOLD_OUT))
            {
                u_first = gesture_data_.u_data[i];
                d_first = gesture_data_.d_data[i];
                l_first = gesture_data_.l_data[i];
                r_first = gesture_data_.r_data[i];
                break;
            }
        }

        /* If one of the _first values is 0, then there is no good data */
        if((u_first == 0) || (d_first == 0) || (l_first == 0) || (r_first == 0) )
            return false;

        /* Find the last value in U/D/L/R above the threshold */
        for( i = gesture_data_.total_gestures - 1; i >= 0; i-- )
        {
            if((gesture_data_.u_data[i]>GESTURE_THRESHOLD_OUT)&&\
               (gesture_data_.d_data[i]>GESTURE_THRESHOLD_OUT)&&\
               (gesture_data_.l_data[i]>GESTURE_THRESHOLD_OUT)&&\
               (gesture_data_.r_data[i]>GESTURE_THRESHOLD_OUT))
            {
                u_last = gesture_data_.u_data[i];
                d_last = gesture_data_.d_data[i];
                l_last = gesture_data_.l_data[i];
                r_last = gesture_data_.r_data[i];
                break;
            }
        }
    }

    /* Calculate the first vs. last ratio of up/down and left/right */
    ud_ratio_first = ((u_first - d_first) * 100) / (u_first + d_first);
    lr_ratio_first = ((l_first - r_first) * 100) / (l_first + r_first);
    ud_ratio_last = ((u_last - d_last) * 100) / (u_last + d_last);
    lr_ratio_last = ((l_last - r_last) * 100) / (l_last + r_last);

    /* Determine the difference between the first and last ratios */
    ud_delta = ud_ratio_last - ud_ratio_first;
    lr_delta = lr_ratio_last - lr_ratio_first;

    /* Accumulate the UD and LR delta values */
    gesture_ud_delta_ += ud_delta;
    gesture_lr_delta_ += lr_delta;

    /* Determine U/D gesture */
    if( gesture_ud_delta_ >= GESTURE_SENSITIVITY_1 )
        gesture_ud_count_ = 1;
    else if( gesture_ud_delta_ <= -GESTURE_SENSITIVITY_1 )
        gesture_ud_count_ = -1;
    else
        gesture_ud_count_ = 0;

    /* Determine L/R gesture */
    if( gesture_lr_delta_ >= GESTURE_SENSITIVITY_1 )
        gesture_lr_count_ = 1;
    else if( gesture_lr_delta_ <= -GESTURE_SENSITIVITY_1 )
        gesture_lr_count_ = -1;
    else
        gesture_lr_count_ = 0;

    /* Determine Near/Far gesture */
    if((gesture_ud_count_ == 0) && (gesture_lr_count_ == 0))
    {
        if((abs(ud_delta) < GESTURE_SENSITIVITY_2) && (abs(lr_delta) < GESTURE_SENSITIVITY_2))
        {
            if((ud_delta == 0) && (lr_delta == 0))
                gesture_near_count_++;
            else if((ud_delta != 0) || (lr_delta != 0))
                gesture_far_count_++;

            if((gesture_near_count_ >= 10) && (gesture_far_count_ >= 2))
            {
                if( (ud_delta == 0) && (lr_delta == 0))
                    gesture_state_ = NEAR_STATE;
                else if((ud_delta != 0) && (lr_delta != 0))
                    gesture_state_ = FAR_STATE;
                return true;
            }
        }
    }
    else
    {
        if((abs((int)ud_delta) < GESTURE_SENSITIVITY_2) && (abs((int)lr_delta) < GESTURE_SENSITIVITY_2))
        {
            if( (ud_delta == 0) && (lr_delta == 0) )
                gesture_near_count_++;
            if( gesture_near_count_ >= 10 )
            {
                gesture_ud_count_ = 0;
                gesture_lr_count_ = 0;
                gesture_ud_delta_ = 0;
                gesture_lr_delta_ = 0;
            }
        }
    }
    return false;
}

/***************************************************************************************************************
*Determines if there is a gesture available for reading
****************************************************************************************************************/
bool APDS9960_isGestureAvailable(void)
{
    uint8_t data;

    /* Read value from GSTATUS register */
    IIC_ReadData(APDS9960_I2C_ADDR, APDS9960_GSTATUS, &data);

    /* Shift and mask out GVALID bit */
    data &= APDS9960_GVALID;

    /* Return true/false based on GVALID bit */
    if( data == 1)
        return true;
    else
        return false;
}

/***************************************************************************************************************
*Determines if there is a gesture available for reading
****************************************************************************************************************/
int32_t APDS9960_readGesture(void)
{
    uint8_t fifo_level = 0;
    int8_t bytes_read = 0;
    uint8_t fifo_data[128];
    uint8_t gstatus;
    int motion;
    int i;

    /* Make sure that power and gesture is on and data is valid */
    if( !APDS9960_isGestureAvailable() || !(getMode() & 0x41) )
        return DIR_NONE;

    /* Keep looping as long as gesture data is valid */
    while(1)
    {
        /* Wait some time to collect next batch of FIFO data */
        DELAY_MS(FIFO_PAUSE_TIME);

        /* Get the contents of the STATUS register. Is data still valid? */
        IIC_ReadData(APDS9960_I2C_ADDR, APDS9960_GSTATUS, &gstatus);

        /* If we have valid data, read in FIFO */
        if( (gstatus & APDS9960_GVALID) == APDS9960_GVALID )
        {
            /* Read the current FIFO level */
            IIC_ReadData(APDS9960_I2C_ADDR, APDS9960_GFLVL, &fifo_level);

            /* If there's stuff in the FIFO, read it into our data block */
            if( fifo_level > 0)
            {
                bytes_read = IIC_ReadDataBlock(APDS9960_I2C_ADDR, APDS9960_GFIFO_U, (uint8_t*)fifo_data, (fifo_level * 4));
                if( bytes_read == -1 )
                return -1;

                /* If at least 1 set of data, sort the data into U/D/L/R */
                if( bytes_read >= 4 )
                {
                    for( i = 0; i < bytes_read; i += 4 )
                    {
                        gesture_data_.u_data[gesture_data_.index] = fifo_data[i + 0];
                        gesture_data_.d_data[gesture_data_.index] = fifo_data[i + 1];
                        gesture_data_.l_data[gesture_data_.index] = fifo_data[i + 2];
                        gesture_data_.r_data[gesture_data_.index] = fifo_data[i + 3];
                        gesture_data_.index++;
                        gesture_data_.total_gestures++;
                    }

                    /* Filter and process gesture data. Decode near/far state */
                    if(APDS9960_processGestureData())
                        if(APDS9960_decodeGesture()){ }

                    /* Reset data */
                    gesture_data_.index = 0;
                    gesture_data_.total_gestures = 0;
                }
            }
        }
        else
        {
            /* Determine best guessed gesture and clean up */
            DELAY_MS(FIFO_PAUSE_TIME);
            APDS9960_decodeGesture();
            motion = gesture_motion_;
            APDS9960_resetGestureParameters();
            return motion;
        }
    }
}



