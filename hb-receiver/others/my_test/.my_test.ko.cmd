cmd_/root/my_test/my_test.ko := ld -r -m elf_x86_64 -T ./scripts/module-common.lds --build-id  -o /root/my_test/my_test.ko /root/my_test/my_test.o /root/my_test/my_test.mod.o
