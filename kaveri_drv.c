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

#define KMS_DRIVER_MAJOR 2
#define KMS_DRIVER_MINOR 41
#define KMS_DRIVER_PATCHLEVEL 0


static struct drm_driver *ddriver;
static struct pci_driver *pdriver;
static struct pci_device_id pciidlist[] = {
    radeon_PCI_IDS
};


//variables
int kaveri_modeset = -1;
int kaveri_gart_size = -1;
int kaveri_vram_limit = 0;
int kaveri_agpmode = 0;
int kaveri_banchmarking = 0;
int kaveri_testing = 0;


int kaveri_device_init(struct radeon_device *rdev,struct drm_device *ddev,struct pci_dev *pdev,uint32_t flags)
{
    int r,i;
    int dma_bits;
    bool runtime = false;

    rdev->shutdown = false;
    rdev->dev = &pdev->dev;
    rdev->ddev = ddev;
    rdev->pdev = pdev;
    rdev->flags = flags;
    rdev->family = flags & RADEON_FAMILY_MASK;
    rdev->is_atom_bios = false;
    rdev->usec_timeout = RADEON_MAX_USEC_TIMEOUT;
    rdev->mc.gtt_size = 512 << 20;
    rdev->accel_working = false;

    for(i = 0;i < RADEON_NUM_RINGS;++ i){
	rdev->ring[i].idx = i;
    }

    rdev->fence_context = fence_context_alloc(RADEON_NUM_RINGS);

    DRM_INFO("kernel modesetting (%s,0x%04X:0x%04X 0x%04X:0x%04X).\n",radeon_family_name[rdev->family],pdev->vendor,pdev->device,pdev->subsystem_vendor,pdev->subsystem_device);

    mutex_init(&rdev->ring_lock);
    mutex_init(&rdev->dc_hw_i2c_mutex);
    atomic_set(&rdev->ih.lock, 0);
    mutex_init(&rdev->gem.mutex);
    mutex_init(&rdev->pm.mutex);
    mutex_init(&rdev->gpu_clock_mutex);
    mutex_init(&rdev->srbm_mutex);
    mutex_init(&rdev->grbm_idx_mutex);
    init_rwsem(&rdev->pm.mclk_lock);
    init_rwsem(&rdev->exclusive_lock);
    init_waitqueue_head(&rdev->irq.vblank_queue);
    mutex_init(&rdev->mn_lock);
    hash_init(rdev->mn_hash);

    INIT_LIST_HEAD(&rdev->gen.objects);

    kaveri_gart_size = 1024;

    rdev->mc.gtt_size = (uint64_t)kaveri_gart_size << 20;

    
    

}


int kaveri_driver_load_kms(strcut drm_device *dev,unsigned long flags)
{
    struct radeon_device *rdev;
    int r,acpi_status;

    rdev = kzalloc(sizeof(struct radeon_device),GFP_KERNEL);
    if(rdev == NULL){
	return -ENOMEM;
    }

    dev->dev_private = (void *)rdev;

    if(drm_pci_device_is_agp(dev)){
	DRM_INFO("kaveri is agp\n");
	flags |= RADEON_IS_AGP;
    }else if(pci_is_pcie(drv_pdev)){
	DRM_INFO("kaveri is pcie\n");
	flags |= RADEON_IS_PCIE;
    }else{
	DRM_INFO("kaveri is pci\n");
	flags |= RADEON_IS_PCI;
    }

    r = kaveri_device_init(rdev,dev,dev->pdev,flags);
    if(r){
	dev_err(&dev->pdev-dev,"Fatal error during GPU init\n");
	goto out;
    }

    r = kaveri_modeset_init(rdev);
    if(r){
	dev_err(&dev->pdev->dev,"Fatal error during modeset init\n");
    }else{
	acpi_status = kaveri_acpi_init(rdev);
	if(acpi_status){
	    dev_dbg(&dev->pdev-dev,"Error during ACPI methods call\n");
	}
    }

    //radeon_kfd_device_probe(rdev);
    //radeon_kfd_device_init(rdev);

 out:
    if(r){
	kaveri_driver_unload_kms(dev);
    }
    return r;
}


static struct drm_driver kaveri_kms_driver = {
    .driver_features = DRIVER_USE_AGP | DRIVER_HAVE_IRQ | DRIVER_IRQ_SHARED | DRIVER_GEM | DRIVER_PRIME | DRIVER RENDER,
    .load = kaveri_driver_load_kms,
    .unload = kaveri_driver_unload_kms,
    .ioctl = kaveri_ioctl_kms,
    .fops = &kaveri_driver_kms_fops,

    
    .name = DRIVER_NAME,
    .desc = DRIVER_DESC,
    .data = DRIVER_DATE,
    .major = KMS_DRIVER_MAJOR,
    .minor = KMS_DRIVER_MINOR,
    .patchlevel = KMS_DRIVER_PATCHLEVEL,
}


static int kaveri_pci_probe(struct pci_dev *pdev,const struct pci_device_id *ent)
{
    int ret;

    //ret = radeon_kick_out_firmware_fb(pdev);

    return drm_get_pci_dev(pdev,ent,&kaveri_kms_driver);
}

static void kaveri_pci_remove(struct pci_dev *pdev)
{
    struct drm_device *dev = pci_get_drvdata(pdev);
    drm_put_dev(dev);
}


static struct pci_driver kaveri_pci_driver = {
    .name = DRIVER_NAME;
    .id_table = pciidlist,
    .probe = kaveri_pci_probe,
    .remove = kaveri_pci_remove,
    //.driver.pm = kaveri_pm_ops,
};

static int __init radeon_init(void)
{
    kaveri_modeset = 1;

    //KMS Init
    DRM_INFO("KMS MODE ON!\n");
    ddriver = &kaveri_kms_driver;
    pdriver = &kaveri_pci_driver;
    ddriver->driver_feature |= DRIVER_MODESET;
    ddriver->num_ioctls = radeon_max_kms_ioctl;
    //radeon_register_atpx_handler();

    //radeon_kfd_init();

    return drm_pci_init(ddriver,pdriver);
}


module_init(radeon_init);
module_exit(radeon_exit);
