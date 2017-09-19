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
	{ 0x710e0d3b, __VMLINUX_SYMBOL_STR(module_layout) },
	{ 0x32740957, __VMLINUX_SYMBOL_STR(simple_attr_release) },
	{ 0x8de200ba, __VMLINUX_SYMBOL_STR(simple_attr_write) },
	{ 0xecc32caa, __VMLINUX_SYMBOL_STR(simple_attr_read) },
	{ 0x359ee64f, __VMLINUX_SYMBOL_STR(generic_file_llseek) },
	{ 0xabb7921, __VMLINUX_SYMBOL_STR(debugfs_remove) },
	{ 0x2a35968d, __VMLINUX_SYMBOL_STR(nf_unregister_hook) },
	{ 0x8b35cde9, __VMLINUX_SYMBOL_STR(debugfs_create_file) },
	{ 0x50e92a92, __VMLINUX_SYMBOL_STR(debugfs_create_u32) },
	{ 0xb7ce9e9b, __VMLINUX_SYMBOL_STR(debugfs_create_dir) },
	{ 0xdcdefc8d, __VMLINUX_SYMBOL_STR(nf_register_hook) },
	{ 0x68466925, __VMLINUX_SYMBOL_STR(simple_attr_open) },
	{ 0xf0fdf6cb, __VMLINUX_SYMBOL_STR(__stack_chk_fail) },
	{ 0x27e1a049, __VMLINUX_SYMBOL_STR(printk) },
	{ 0x91715312, __VMLINUX_SYMBOL_STR(sprintf) },
	{ 0xbdfb6dbb, __VMLINUX_SYMBOL_STR(__fentry__) },
};

static const char __module_depends[]
__used
__attribute__((section(".modinfo"))) =
"depends=";


MODULE_INFO(srcversion, "2D97AFBDC136F21BAE4B4F1");
