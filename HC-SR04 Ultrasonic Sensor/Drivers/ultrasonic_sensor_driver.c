#include "ultrasonic_sensor_driver.h"

static ultrasonic_Handle_t* sensor = NULL; //Pointer used by the callback (Initialized to NULL for safety).

void Ultrasonic_Sensor_Init(ultrasonic_Handle_t* ultrasonic_sensor) //User will declare parameters in the main.c and call this function to initialize the sensor.
{
	sensor = ultrasonic_sensor; 
	ultrasonic_sensor->echo_state = WAITING_FOR_RISING; //Initially, the board is waiting for a rising edge on the ECHO Pin.
	ultrasonic_sensor->ultrasonic_ready = 0; //No data is ready to be fetched yet.
    HAL_TIM_IC_Start_IT(ultrasonic_sensor->htim_echo, ultrasonic_sensor->echo_channel); //Starts input-capture interrupt on the default polarity configured in CubeMX (usually rising edge).
}
ultrasonic_status_t Ultrasonic_Trigger(ultrasonic_Handle_t* ultrasonic_sensor){ //Send a single pulse to the ultrasonic sensor's TRIG pin.
	if(ultrasonic_sensor->ultrasonic_ready ||
			ultrasonic_sensor->echo_state == WAITING_FOR_FALLING){ 
		return ULTRASONIC_BUSY;  //If there is data that hasn't been fetched yet (to avoid overwriting it) or the input capture on the TIM channel is waiting for a falling edge, return ULTRASONIC_BUSY
	}
	else{
		ultrasonic_sensor->ultrasonic_ready = 0; //Clear timestamps to avoid mixing old and new measurements.
		ultrasonic_sensor->t_fall = 0; //Clear old t_fall value.
		ultrasonic_sensor->t_rise = 0; //Clear old t_rise value.
		__HAL_TIM_DISABLE(ultrasonic_sensor->htim_trig); //Stop the timer that generates the TRIG pulse so we can clear it.
		__HAL_TIM_SET_COUNTER(ultrasonic_sensor->htim_trig, 0); //Reset the CNT register to start counting again.
		HAL_TIM_PWM_Start(ultrasonic_sensor->htim_trig, ultrasonic_sensor->trig_channel); //Arm the output channel.
		__HAL_TIM_ENABLE(ultrasonic_sensor->htim_trig); //Enable timer back again to start counting.
		//Important note: HAL_TIM_PWM_Start() can't send an output unless the counter register is enabled, so it arms enables the channel output and prepares CCR,ARR preload. Output begins only after we use the __HAL_TIM_ENABLE() to enable the count regiSter again.  
	}
	return ULTRASONIC_OK; //Return ULTRASONIC_OK if it all went well.
}

void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim) //Defining the Input Capture callback in non-blocking mode. (It triggers automatically after the input capture detects a rising/falling edge)
{
	if(sensor == NULL){ //Callback may fire even before initialization if interrupts are enabled early.
		return;
	}
	else if(htim == sensor->htim_echo){ //If sensor is correctly declared, we can then proceed to deal with our definition.
    	if(sensor->echo_state == WAITING_FOR_RISING){ //If the input capture is set to waiting for a rising edge, that means we must switch it to falling edge to detect when the sensor's ECHO pin goes from HIGH to LOW (sound wave returned).
    		sensor->t_rise = __HAL_TIM_GET_COMPARE(sensor->htim_echo,sensor->echo_channel);//Read the time in which the input capture detected a rising edge.(Data is read from the CCRX-Capture and Compare Register X)
    		TIM_IC_InitTypeDef sConfig= {0}; //Declare an sConfig structure to change the attributes of the capture input channel. (Specifically to the edge detection polarity)
    		sConfig.ICPolarity = TIM_INPUTCHANNELPOLARITY_FALLING; //Tells the timer which edge it latch the CNT register to the CCR.
    		sConfig.ICSelection = TIM_ICSELECTION_DIRECTTI; //Use the direct input pin TIx as the capture source.
    		sConfig.ICFilter = 0; //Does not add digital noise filtering (Ultrasonic is usually not noisy).
			sConfig.ICPrescaler = TIM_ICPSC_DIV1; //Sets how many valid edges are required to capture an input (DIVN = Wait for the N-th Edge and trigger interrupt).
			HAL_TIM_IC_ConfigChannel(sensor->htim_echo,&sConfig,sensor->echo_channel); //Load the new configuration into the timer channel.
			HAL_TIM_IC_Start_IT(sensor->htim_echo,sensor->echo_channel); //Start the TIM input capture in interrupt mode again (This time waiting for a falling edge).
    		sensor->echo_state = WAITING_FOR_FALLING; //Now the input capture pin is waiting for a falling edge so it can read the actual distance.
    	}
    	else if(sensor->echo_state == WAITING_FOR_FALLING){ //If the input capture is waiting for a falling edge, that means we need to read the time and switch back to the rising edge polarity. But we must inform the user/board that there is information that is waiting be fetched.
    	    		sensor->t_fall = __HAL_TIM_GET_COMPARE(sensor->htim_echo,sensor->echo_channel); //Read the time in which the input capture detected a falling edge. (Data is read from the CCRX-Capture and Compare Register X)
    	    		TIM_IC_InitTypeDef sConfig = {0}; //Declare an sConfig structure to change the attributes of the capture input channel. (Specifically to the edge detection polarity)
    	    		sConfig.ICFilter = 0; //Does not add digital noise filtering (Ultrasonic is usually not noisy).
    	    		sConfig.ICPolarity =  TIM_INPUTCHANNELPOLARITY_RISING; //Set edge at which we capture the value of CNT to rising edge. 
    	    		sConfig.ICPrescaler = TIM_ICPSC_DIV1; //Capture the CNT value after 1 edge detection.
    	    		sConfig.ICSelection = TIM_ICSELECTION_DIRECTTI; //Selects the actual pin as input capture.
    	    		HAL_TIM_IC_ConfigChannel(sensor->htim_echo,&sConfig,sensor->echo_channel); //Load the new configuration into the timer channel.
    	    		HAL_TIM_IC_Start_IT(sensor->htim_echo,sensor->echo_channel); //Start the input capture process again.
    	    		sensor->ultrasonic_ready = 1; //Set the ultrasonic_ready as 1 to block any operation that tries to trigger the sensor before data is fetched by the user/board.
    	    		sensor->echo_state = WAITING_FOR_RISING; //Now the input capture pin is waiting for the next rising edge to start the reading cycle again.
    	    	}
    }
	else{
		return; //Empty return
	}
}

uint8_t Ultrasonic_IsReady(ultrasonic_Handle_t* ultrasonic_sensor){
	return ultrasonic_sensor->ultrasonic_ready; 
}
float Ultrasonic_GetDistance(ultrasonic_Handle_t* ultrasonic_sensor){
	if(Ultrasonic_IsReady(ultrasonic_sensor)==0){ //If data cannot be fetched yet, return "Not A Number" (NAN requires math.h header).
		return NAN;
	}
	else{ //Calculate the actual distance. (Distance = Speed of sound * Difference(t_fall - t_rise) /2)
		uint16_t time_difference = ultrasonic_sensor->t_fall - ultrasonic_sensor->t_rise ;
		ultrasonic_sensor->ultrasonic_ready = 0;
		float distance = (time_difference / 2.0f) * 0.0343f; //0.0343 m/s is speed of sound at ~20 - 25C.
		return distance;
	}
}
