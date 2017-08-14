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
	{ 0x68742e9b, __VMLINUX_SYMBOL_STR(send_check_tlb) },
	{ 0x209d8d22, __VMLINUX_SYMBOL_STR(send_check_tlb_lock) },
	{ 0x68e2f221, __VMLINUX_SYMBOL_STR(_raw_spin_unlock) },
	{ 0x51fe6379, __VMLINUX_SYMBOL_STR(inet_sendmsg_tlb) },
	{ 0x67f7403e, __VMLINUX_SYMBOL_STR(_raw_spin_lock) },
	{ 0xda22b298, __VMLINUX_SYMBOL_STR(inet_sendmsg_tlb_lock) },
	{ 0x6bf1c249, __VMLINUX_SYMBOL_STR(inet_sendmsg_tlb_index) },
	{ 0x2ec2d4fe, __VMLINUX_SYMBOL_STR(send_check_tlb_index) },
	{ 0x27e1a049, __VMLINUX_SYMBOL_STR(printk) },
};

static const char __module_depends[]
__used
__attribute__((section(".modinfo"))) =
"depends=hb_sender_tracker";


MODULE_INFO(srcversion, "12C28BA2C18156F4F2D6C00");
