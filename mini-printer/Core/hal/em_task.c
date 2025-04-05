#include "em_task.h"
#include "em_button.h"
#include "em_motor.h"
#include "em_printer.h"
#include "em_adc.h"

bool printer_test = false;
bool is_long_click_start = false;

void key_click_handle()
{
    printf("Button Clicked!\n");
    printer_test = true;
    // read_all_hal();
}

void key_long_click_handle()
{
		if(is_long_click_start == true)
			return;
		is_long_click_start = true;
    printf("Button long pressed!\n");
    device_state_t *pdevice = get_device_state();
    bool need_beep = false;
    // ��ȱֽ�Ҳ��ڴ�ӡ�в�ִ��
    if (pdevice->paper_state == PAPER_STATUS_NORMAL)
    {
        if (pdevice->printer_state == PRINTER_STATUS_FINISH ||
            pdevice->printer_state == PRINTER_STATUS_INIT)
        {
            printf("Outputing paper \n");
            motor_start();
        }
        else
        {
            need_beep = true;
        }
    }
    else
        need_beep = true;
    if (need_beep)
    {
        run_beep(BEEP_WARN);
				run_led(LED_WARN);
    }
}

void key_long_click_free_handle()
{
		is_long_click_start = false;
    printf("Stop paper output\n");
    motor_stop();
}


/**
 * @brief report event
 *
 */
void run_report()
{
    if (get_state_timeout())
    {
				clean_state_timeout();
        read_all_hal();
        if (get_ble_connect())
        {
						printf("report device status:report time up\n");
            ble_report();
        }
    }
    if (read_paper_irq_need_report_status())
    {
        // no paper interrupt, report!
        printf("report device status : paper irq\n");
        ble_report();
    }
}

/**
 * @brief printing revelent event handler
 *
 */
void run_printer()
{
    device_state_t *pdevice = get_device_state();
		#ifdef START_PRINTER_WHEN_FINISH_RAED
        if (pdevice->read_ble_finish == true)
        {
            if (pdevice->printer_state == PRINTER_STATUS_FINISH ||
                pdevice->printer_state == PRINTER_STATUS_INIT)
            {
                pdevice->read_ble_finish = false;
                pdevice->printer_state = PRINTER_STATUS_START;
                ble_report();
                printf("report device status : printing start %d\n",get_ble_rx_leftline());
                run_beep(BEEP_PRINTER_START);
                run_led(LED_PRINTER_START);
            }
        }
    #else
			// triger the printing when recived more than 200 lines
			if (get_ble_rx_leftline()> 200)
			{
					if (pdevice->printer_state == PRINTER_STATUS_FINISH ||
							pdevice->printer_state == PRINTER_STATUS_INIT)
					{
							pdevice->printer_state = PRINTER_STATUS_START;
							ble_report();
							printf("report device status : printing start\n");
							run_beep(BEEP_PRINTER_START);
							run_led(LED_PRINTER_START);
					}
			}
		#endif
    // start printing
    if (pdevice->printer_state == PRINTER_STATUS_START)
    {
        start_printing_by_queuebuf();
        pdevice->printer_state = PRINTER_STATUS_FINISH;
    }
}

void task_report(void *pvParameters)
{
		int count = 0;
		printf("task_report init\n");
    for (;;) // A Task shall never return or exit.
    {
        run_report();
        vTaskDelay(100);
				count ++;
				if(count >= 50){
					count = 0;
					printf("task_report run\n");
				}
    }
}

void task_button(void *pvParameters)
{
		int count = 0;
		printf("task_button init\n");
    for (;;) // A Task shall never return or exit.
    {
        key_check_run(); // The button handling function needs to be called periodically.
        vTaskDelay(20);
				count ++;
				if(count >= 250){
					count = 0;
					printf("task_button run\n");
				}
    }
}

void task_printer(void *pvParameters)
{
		int count = 0;
		init_ble();
		printf("task_printer init\n");
    for (;;) // A Task shall never return or exit.
    {
				ble_status_data_clean();
        run_printer();
        vTaskDelay(1);
        if(printer_test){
            printer_test = false;
            testSTB();
        }
				count ++;
				if(count >= 5000){
					count = 0;
					printf("task_printer run\n");
				}
    }
}

//void printer_run(){
//    // printer_test = true;
//    for (;;) // A Task shall never return or exit.
//    {
//        run_printer();
//        // vTaskDelay(10);
//        if(printer_test){
//            printer_test = false;
//            testSTB();
//        }
//    }
//}

void init_task()
{
    printf("init_task\n");
    init_device_state();
    init_timer();
    init_hal();
    init_queue();
    adc_init();
    init_key();
    init_printer();
    //
    xTaskCreate(
        task_report,  // Task function
        "TaskReport", // Task name
        128,          // Task stack size
        NULL,         // Task parameters
        1,            // Task priority; with 3 (configMAX_PRIORITIES - 1) being the highest, 0 the lowest
        NULL          // Task handle
    );

    xTaskCreate(
        task_button,  // Task function
        "TaskButton", // Task name
        128,          // Task stack size
        NULL,         // Task parameters
        0,            // Task priority; with 3 (configMAX_PRIORITIES - 1) being the highest, 0 the lowest
        NULL          // Task handle
    );

    // xTaskCreate(
    //     task_printer,  // Task function
    //     "TaskPrinter", // Task name
    //     256,           // Task stack size
    //     NULL,          // Task parameters
    //     2,             // Task priority; with 3 (configMAX_PRIORITIES - 1) being the highest, 0 the lowest
    //     NULL           // Task handle
    // );
}


