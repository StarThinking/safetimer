#include <linux/init.h>
#include <linux/module.h> 
#include <linux/sched.h>
#include <linux/rcupdate.h>
#include <linux/fdtable.h>
#include <linux/fs.h> 
#include <linux/fs_struct.h>
#include <linux/dcache.h>
#include <linux/slab.h>
#include <linux/net.h>
#include <linux/pid.h>
#include <linux/udp.h>
#include <net/inet_sock.h>

MODULE_LICENSE("GPL");

static int p_id = 12510;
static struct pid *pid_struct;
static struct task_struct *task;

static int process_port_map_init(void) { 
        struct files_struct *task_files; 
        struct fdtable *files_table;
        struct path files_path;
        int i = 0;
        char *cwd;
        char *buf = (char *)kmalloc(GFP_KERNEL, 100*sizeof(char));
        
        pid_struct = find_get_pid(p_id);
        task = pid_task(pid_struct, PIDTYPE_PID);
        task_files = task->files;
        files_table = files_fdtable(task_files);

        while (files_table->fd[i] != NULL) {
                struct socket *sock;
                int err;
                struct inet_sock *inet;
                unsigned int sport, dport;

                sock = sock_from_file(files_table->fd[i], &err);
                if (sock != NULL) {
                        printk("Socket exists\n");
                        
                        inet = inet_sk(sock->sk);
                        sport = htons(inet->inet_sport);
                        dport = htons(inet->inet_dport);
                        
                        printk("%pI4 : %u  -> %pI4 : %u\n", 
                                &(inet->inet_rcv_saddr), sport, &(inet->inet_daddr), dport);
                }
                files_path = files_table->fd[i]->f_path;
                cwd = d_path(&files_path, buf, 100*sizeof(char));

                printk("Open file with fd %d %s\n", i, cwd);
                i++;
        }
        return 0; 
} 

static void process_port_map_exit(void) { 
        printk("process_port_map_exit");
} 

module_init(process_port_map_init);
module_exit(process_port_map_exit);
