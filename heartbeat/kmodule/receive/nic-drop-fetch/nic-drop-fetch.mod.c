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
	{ 0xc9aca5d0, __VMLINUX_SYMBOL_STR(debugfs_remove_recursive) },
	{ 0x8b35cde9, __VMLINUX_SYMBOL_STR(debugfs_create_file) },
	{ 0xb7ce9e9b, __VMLINUX_SYMBOL_STR(debugfs_create_dir) },
	{ 0xf0fdf6cb, __VMLINUX_SYMBOL_STR(__stack_chk_fail) },
	{ 0x619cb7dd, __VMLINUX_SYMBOL_STR(simple_read_from_buffer) },
	{ 0x91715312, __VMLINUX_SYMBOL_STR(sprintf) },
	{ 0xe484e35f, __VMLINUX_SYMBOL_STR(ioread32) },
	{ 0xd2d7216a, __VMLINUX_SYMBOL_STR(my_tp_regs) },
	{ 0x79c11397, __VMLINUX_SYMBOL_STR(my_tp_regs_count) },
	{ 0xbdfb6dbb, __VMLINUX_SYMBOL_STR(__fentry__) },
};

static const char __module_depends[]
__used
__attribute__((section(".modinfo"))) =
"depends=tg3";


MODULE_INFO(srcversion, "1939E6067C5B361C650F266");