struct kernel_drop_stats {
        long udp_errors;
};

void init_kernel_drop(struct kernel_drop_stats *stats);

int check_kernel_drop(struct kernel_drop_stats *last_stats);
