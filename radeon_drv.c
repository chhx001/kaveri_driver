/*
  Driver for training.
  Only for Kaveri.
  
  By,
  Horace Chen.
  April 29th, 2015
*/
  

#include <drm/drmP.h>
#include <drm/radeon_drm.h>

#include <drm/drm_pciids.h>
#include <linux/console.h>
#include <linux/module.h>
#include <linux/pm_runtime.h>
#include <linux/vga_switcheroo.h>
#include <drm/drm_gem.h>



#define DRIVER_AUTOHR "Horace Chen"
#define DRIVER_NAME "kaveri"
#define DRIVER_DESC "Radeon Driver Trainning, Only Made for Kaveri"
#define DRIVER_DATE "20150429"



static struct drm_driver *ddriver;
static struct pci_driver *pdriver;
static struct pci_device_id pciidlist[] = {
    radeon_PCI_IDS
};

static struct pci_driver kaveri_pci_driver = {
    .name = DRIVER_NAME;
    .id_table = pciidlist,
    .probe = kaveri_probe,
    .remove = kaveri_remove,
    .driver.pn = kaveri_pm_ops,
};

static int __init radeon_init(void)
{
    radeon_modeset = 1;

    //KMS Init
    DRM_INFO("KMS MODE ON!\n");
    ddriver = &kaveri_kms_driver;
    pdriver = &kaveri_pci_driver;
    ddriver->driver_feature |= DRIVER_MODESET;
    ddriver->num_ioctls = radeon_max_kms_ioctl;
    //radeon_register_atpx_handler();

    radeon_kfd_init();

    return drm_pci_init(ddriver,pdriver);
}


module_init(radeon_init);
module_exit(radeon_exit);
