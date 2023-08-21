/*
 *--------------------------------------
 * Program Name:
 * Author:
 * License:
 * Description:
 *--------------------------------------
*/

#include <srldrvce.h>
#include <graphx.h>
#include <debug.h>
#include <keypadc.h>
#include <stdbool.h>
#include <string.h>

srl_device_t srl;

bool has_srl_device = false;

uint8_t srl_buf[512];

uint8_t line = 0;

void printCenter(const char *str, const char *error);

static usb_error_t handle_usb_event(usb_event_t event, void *event_data,
                                    usb_callback_data_t *callback_data __attribute__((unused))) {
    usb_error_t err;
    /* Delegate to srl USB callback */
    if ((err = srl_UsbEventCallback(event, event_data, callback_data)) != USB_SUCCESS)
        return err;
    /* Enable newly connected devices */
    if(event == USB_DEVICE_CONNECTED_EVENT && !(usb_GetRole() & USB_ROLE_DEVICE)) {
        usb_device_t device = event_data;
        printCenter("device connected", NULL);
        usb_ResetDevice(device);
    }

    /* Call srl_Open on newly enabled device, if there is not currently a serial device in use */
    if(event == USB_HOST_CONFIGURE_EVENT || (event == USB_DEVICE_ENABLED_EVENT && !(usb_GetRole() & USB_ROLE_DEVICE))) {

        /* If we already have a serial device, ignore the new one */
        if(has_srl_device) return USB_SUCCESS;

        usb_device_t device;
        if(event == USB_HOST_CONFIGURE_EVENT) {
            /* Use the device representing the USB host. */
            device = usb_FindDevice(NULL, NULL, USB_SKIP_HUBS);
            if(device == NULL) return USB_SUCCESS;
        } else {
            /* Use the newly enabled device */
            device = event_data;
        }

        /* Initialize the serial library with the newly attached device */
        srl_error_t error = srl_Open(&srl, device, srl_buf, sizeof srl_buf, SRL_INTERFACE_ANY, 9600);
        if(error) {
            /* Print the error code to the homescreen */
            printCenter("Error initting serial", error);
            return USB_SUCCESS;
        }

        printCenter("serial initialized", NULL);

        has_srl_device = true;
    }

    if(event == USB_DEVICE_DISCONNECTED_EVENT) {
        usb_device_t device = event_data;
        if(device == srl.dev) {
            printCenter("device disconnected", NULL);
            srl_Close(&srl);
            has_srl_device = false;
        }
    }

    return USB_SUCCESS;
}


void printCenter(const char *str, const char *error)
{
    if (error) {
        gfx_PrintStringXY(str, 0, line * 8);
        line += 1;
        gfx_PrintStringXY(error, 0, line * 8);
        line += 1;
    } else {
        gfx_PrintStringXY(str, 0, line * 8);
        line += 1;
    }
}


int main(void) {

    gfx_Begin();

    const usb_standard_descriptors_t *desc = srl_GetCDCStandardDescriptors();
    /* Initialize the USB driver with our event handler and the serial device descriptors */
    usb_error_t usb_error = usb_Init(handle_usb_event, NULL, desc, USB_DEFAULT_INIT_FLAGS);
    if(usb_error) {
       usb_Cleanup();
       gfx_End();
       printCenter("usb init error", usb_error);
       do kb_Scan(); while(!kb_IsDown(kb_KeyClear));
       return 1;
    }

    do {
        kb_Scan();
        
        usb_HandleEvents();

        if(has_srl_device) {
            char in_buf[64];

            /* Read up to 64 bytes from the serial buffer */
            size_t bytes_read = srl_Read(&srl, in_buf, sizeof in_buf);

            /* Check for an error (e.g. device disconneced) */
            if(bytes_read < 0) {
                printCenter("error on srl_Read", bytes_read);
                has_srl_device = false;
            } else if(bytes_read > 0) {
                printCenter(in_buf, NULL); // Output calc data

                /* Write the data back to serial */
                srl_Write(&srl, in_buf, bytes_read);
            }
        }

    } while(!kb_IsDown(kb_KeyClear));

    usb_Cleanup();
    gfx_End();
    return 0;
}