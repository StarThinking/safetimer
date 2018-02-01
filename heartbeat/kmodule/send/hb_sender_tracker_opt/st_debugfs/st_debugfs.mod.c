#include <linux/module.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

MODULE_INFO(vermagic, VERMAGIC_STRING);

__visible struct module __this_module
__attribute__((section(".gnu.linkonce.this_module"))) = {
	.name = KBUILD_MODNAME,
	.init = init_module,
#ifdef CONFIG_MODULE_UNLOAD
	.exit = cleanup_module,
#endif
	.arch = MODULE_ARCH_INIT,
};

static const struct modversion_info ____versions[]
__used
__attribute__((section("__versions"))) = {
	{ 0x9d35aeec, __VMLINUX_SYMBOL_STR(module_layout) },
	{ 0x619cb7dd, __VMLINUX_SYMBOL_STR(simple_read_from_buffer) },
	{ 0xd77d8c3d, __VMLINUX_SYMBOL_STR(generic_file_llseek) },
	{ 0x9313d603, __VMLINUX_SYMBOL_STR(debugfs_create_dir) },
	{ 0x779a18af, __VMLINUX_SYMBOL_STR(kstrtoll) },
	{ 0xbc82831d, __VMLINUX_SYMBOL_STR(debugfs_create_file) },
	{ 0x3c050e81, __VMLINUX_SYMBOL_STR(debugfs_remove_recursive) },
	{ 0xba8e9b4f, __VMLINUX_SYMBOL_STR(simple_attr_read) },
	{ 0x27e1a049, __VMLINUX_SYMBOL_STR(printk) },
	{ 0x8043dd5e, __VMLINUX_SYMBOL_STR(simple_attr_release) },
	{ 0xbdfb6dbb, __VMLINUX_SYMBOL_STR(__fentry__) },
	{ 0xbb4f4766, __VMLINUX_SYMBOL_STR(simple_write_to_buffer) },
	{ 0x5ed8d375, __VMLINUX_SYMBOL_STR(simple_attr_open) },
	{ 0x8783aed8, __VMLINUX_SYMBOL_STR(simple_attr_write) },
};

static const char __module_depends[]
__used
__attribute__((section(".modinfo"))) =
"depends=";


MODULE_INFO(srcversion, "19193B67ACAACF0BD787632");
