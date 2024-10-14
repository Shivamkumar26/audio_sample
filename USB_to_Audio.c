#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/usb.h>
#include <sound/core.h>
#include <sound/initval.h>
#include <sound/pcm.h>

MODULE_AUTHOR("Shivam");
MODULE_DESCRIPTION("USB Audio Driver for Waveshare USB Audio on Raspberry Pi 4");
MODULE_LICENSE("GPL");

static struct usb_device_id snd_my_audio_ids[] = {
    { .match_flags = USB_DEVICE_ID_MATCH_VENDOR | USB_DEVICE_ID_MATCH_PRODUCT,
      .idVendor = 0x0c76, /* Replace with your Waveshare device's vendor ID */
      .idProduct = 0x1203, /* Replace with your Waveshare device's product ID */
    },
    { } /* Terminating entry */
};
MODULE_DEVICE_TABLE(usb, snd_my_audio_ids);

static int snd_my_audio_probe(struct usb_interface *intf,
                               const struct usb_device_id *id)
{
    printk(KERN_INFO "My USB Audio device (%04x:%04x) plugged\n",
           id->idVendor, id->idProduct);
    return 0;
}

static void snd_my_audio_disconnect(struct usb_interface *intf)
{
    printk(KERN_INFO "My USB Audio device unplugged\n");
}

static struct usb_driver snd_my_audio_driver = {
    .name = "my_usb_audio",
    .probe = snd_my_audio_probe,
    .disconnect = snd_my_audio_disconnect,
    .id_table = snd_my_audio_ids,
};

static int __init snd_my_audio_init(void)
{
printk(KERN_INFO "INIT function\n");
return usb_register(&snd_my_audio_driver);
}

static void __exit snd_my_audio_exit(void)
{
   printk(KERN_INFO "EXIT function\n"); 
usb_deregister(&snd_my_audio_driver);
}

module_init(snd_my_audio_init);
module_exit(snd_my_audio_exit);
