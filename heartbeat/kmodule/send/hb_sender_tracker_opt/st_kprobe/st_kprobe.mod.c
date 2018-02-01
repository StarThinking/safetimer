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
	{ 0x638fe045, __VMLINUX_SYMBOL_STR(unregister_kprobe) },
	{ 0xd3ec7d38, __VMLINUX_SYMBOL_STR(pid_task) },
	{ 0xf0f30986, __VMLINUX_SYMBOL_STR(find_pid_ns) },
	{ 0x7a4d3cc3, __VMLINUX_SYMBOL_STR(st_pid) },
	{ 0xc314828f, __VMLINUX_SYMBOL_STR(init_pid_ns) },
	{ 0x512b1d19, __VMLINUX_SYMBOL_STR(register_kprobe) },
	{ 0x27e1a049, __VMLINUX_SYMBOL_STR(printk) },
	{ 0xc34a7312, __VMLINUX_SYMBOL_STR(send_sig_info) },
	{ 0xbdfb6dbb, __VMLINUX_SYMBOL_STR(__fentry__) },
};

static const char __module_depends[]
__used
__attribute__((section(".modinfo"))) =
"depends=st_debugfs";


MODULE_INFO(srcversion, "392706375065094B95D1C47");
