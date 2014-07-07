#include <linux/types.h>
#include <linux/kobject.h>
#include <linux/stddef.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/ctype.h>
#include <linux/ktime.h>
#include <linux/sched.h>
#include <linux/delay.h>
#include <linux/freezer.h>

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))
#endif

#define NAME_LEN			64
#define DELAY_300_SECS		300

ktime_t ktime_get(void);
int kernel_is_in_earlysuspend(void);
static char search_name[NAME_LEN];
static int search_pid;
static long search_pid_time;

static char anc_search_name[NAME_LEN];
static int anc_search_pid;
static long anc_search_pid_time;

// for external
int blacklist_debug = 0;

struct pkg_list {
	struct pkg_list *next;
	char uid[NAME_LEN];
	int disconnected;
};
static struct pkg_list *pkgl;

struct ap_network_count_list {
	struct ap_network_count_list *next;
	char uid[NAME_LEN];
	int count;
};
static struct ap_network_count_list *ancl;

static DEFINE_MUTEX(tree_lock);
static DEFINE_MUTEX(anc_tree_lock);

struct init_pkg_list {
	char uid[NAME_LEN];
	int disconnected;
};

// white list
/*
struct init_pkg_list default_ap_list[] = {
	{"RX_Thread", 0},
	{"dnsmasq", 0},
	{"netd", 0},
	{"system_server", 0},
	{"android.process.acore", 0},
	{"android.process.media", 0},
	{"com.process.phone", 0},
	{"com.android.contacts", 0},
	{"com.android.launcher", 1},
	{"com.android.systemui", 1},
	{"com.google.android.calendar", 0},
	{"com.android.deskclock", 0},
	{"jp.naver.android.npush", 0},
	{"jp.naver.line.android", 0},
};
*/

// black list
struct init_pkg_list default_ap_list[] = {
	{"NULL", 0},
};

//====================================
// ap network count list
//====================================

void anc_clean_count(void)
{
	struct ap_network_count_list *move;

	mutex_lock(&anc_tree_lock);
	for (move = ancl; move != NULL; move = move->next) {
		move->count = 0;
	}
	mutex_unlock(&anc_tree_lock);
}

struct ap_network_count_list * anc_search(const char *uid)
{
	struct ap_network_count_list *move;

	mutex_lock(&anc_tree_lock);
	for (move = ancl; move != NULL; move = move->next) {
		//if (!strcmp(move->uid, uid)) {
		if (strstr(uid, move->uid)) {
			mutex_unlock(&anc_tree_lock);
			return move;
		}
	}
	mutex_unlock(&anc_tree_lock);
	return NULL;
}

void anc_add(const char *uid)
{
	struct ap_network_count_list *node = NULL;
	struct ap_network_count_list *move = NULL;

	if (uid == NULL)
		return;

	if ((node = anc_search(uid))) {
		node->count++;
		return;
	}

	mutex_lock(&anc_tree_lock);

	node = kmalloc(sizeof(struct ap_network_count_list), GFP_KERNEL);
	if (node == NULL) {
		mutex_unlock(&anc_tree_lock);
		return;
	}

	strncpy(node->uid, uid, NAME_LEN - 1);
	node->uid[sizeof(node->uid)-1] = 0;
	node->count = 1;
	node->next = NULL;

	if (ancl == NULL)
		ancl = node;
	else {
		for (move = ancl; move != NULL; move = move->next)
			if (move->next == NULL)
				break;
		if (move)
			move->next = node;
	}
	mutex_unlock(&anc_tree_lock);
}

void anc_remove(const char *uid)
{
	struct ap_network_count_list *move, *head;

	mutex_lock(&anc_tree_lock);
	for (head = move = ancl; move != NULL; move = move->next) {
		if (!strcmp(move->uid, uid)) {
			if (head == move) {
				head = move->next;
				ancl = head;
			}
			else if (head != move) {
				head->next = move->next;
			}
			kfree(move);
			mutex_unlock(&anc_tree_lock);
			return ;
		}
		else
			head = move;
	}
	mutex_unlock(&anc_tree_lock);
}

ssize_t  network_count_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	char *s = buf;
	char *end = buf + PAGE_SIZE;
	struct ap_network_count_list *node;

	if (blacklist_debug)
		pr_info("[BlackList] nc  name=%s\r\n", anc_search_name);
	if (anc_search_name[0] == 0) {
		anc_search_pid = 0;
		s += scnprintf(s, end - s, "0");
		return (s - buf);
	}

	if ((node = anc_search(anc_search_name))) {
		if (blacklist_debug)
			pr_info("[BlackList] nc  name=%s   found!!!!\r\n", anc_search_name);
		anc_search_name[0] = 0;
		anc_search_pid = 0;
		s += scnprintf(s, end - s, "%d", node->count);
		return (s - buf);
	}

	anc_search_name[0] = 0;
	anc_search_pid = 0;
	s += scnprintf(s, end - s, "0");
	return (s - buf);
}

ssize_t  network_count_store(struct kobject *kobj, struct kobj_attribute *attr,
			const char *buf, size_t n)
{
	const char *arg;
	int name_len = 0;
	unsigned long long t;
	unsigned long nanosec_rem;

	arg = buf;
	while (*arg && !isspace(*arg))
		arg++;
	name_len = arg - buf;
	if (!name_len)
		goto bad_name;
	if (name_len >= NAME_LEN)
		name_len = NAME_LEN - 1;

	if (anc_search_pid == 0)
		anc_search_pid = current->pid;
	else {
		t = cpu_clock(0);
		nanosec_rem = do_div(t, 1000000000);
		while(anc_search_pid && (t - anc_search_pid_time < 3))
			msleep(300);
		anc_search_pid = current->pid;
	}
	t = cpu_clock(0);
	nanosec_rem = do_div(t, 1000000000);
	anc_search_pid_time = t;

	strncpy(anc_search_name, buf, name_len);
	anc_search_name[name_len] = 0;

bad_name:
	return n;
}

ssize_t  network_count_reset_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	char *s = buf;
	char *end = buf + PAGE_SIZE;

	s += scnprintf(s, end - s, "0");
	return (s - buf);
}

ssize_t  network_count_reset_store(struct kobject *kobj, struct kobj_attribute *attr,
			const char *buf, size_t n)
{
	anc_clean_count();
	return n;
}

//====================================
// black list
//====================================

void pkgl_clean_disconnect(void)
{
	struct pkg_list *move;

	for (move = pkgl; move != NULL; move = move->next) {
		move->disconnected = 0;
	}
}

int pkgl_set_disconnect(const char *uid)
{
	struct pkg_list *move;

	mutex_lock(&tree_lock);
	for (move = pkgl; move != NULL; move = move->next) {
		if (strstr(move->uid, uid)) {
			move->disconnected = 1;
			mutex_unlock(&tree_lock);
			return 1;
		}
	}
	mutex_unlock(&tree_lock);
	return 0;
}

int pkgl_query_disconnect(const char *uid)
{
	struct pkg_list *move;

	mutex_lock(&tree_lock);
	for (move = pkgl; move != NULL; move = move->next) {
		if (strstr(move->uid, uid)) {
			if (move->disconnected == 1) {
				mutex_unlock(&tree_lock);
				return 1;
			}
			else {
				mutex_unlock(&tree_lock);
				return 0;
			}
		}
	}
	mutex_unlock(&tree_lock);
	return 0;
}

struct pkg_list * pkgl_search(const char *uid)
{
	struct pkg_list *move;

	mutex_lock(&tree_lock);
	for (move = pkgl; move != NULL; move = move->next) {
		if (!strcmp(move->uid, uid)) {
			mutex_unlock(&tree_lock);
			return move;
		}
	}
	mutex_unlock(&tree_lock);
	return NULL;
}

void pkgl_add(const char *uid, int block)
{
	struct pkg_list *node = NULL;
	struct pkg_list *move = NULL;

	if (uid == NULL)
		return;

	if (pkgl_search(uid)) {
		return;
	}

	mutex_lock(&tree_lock);

	node = kmalloc(sizeof(struct pkg_list), GFP_KERNEL);
	if (node == NULL) {
		mutex_unlock(&tree_lock);
		return;
	}

	strncpy(node->uid, uid, NAME_LEN - 1);
	node->uid[sizeof(node->uid)-1] = 0;
	node->disconnected = block;
	node->next = NULL;

	if (pkgl == NULL)
		pkgl = node;
	else {
		for (move = pkgl; move != NULL; move = move->next)
			if (move->next == NULL)
				break;
		if (move)
			move->next = node;
	}
	mutex_unlock(&tree_lock);
}

void pkgl_remove(const char *uid)
{
	struct pkg_list *move, *head;

	mutex_lock(&tree_lock);
	for (head = move = pkgl; move != NULL; move = move->next) {
		if (!strcmp(move->uid, uid)) {
			if (head == move) {
				head = move->next;
				pkgl = head;
			}
			else if (head != move) {
				head->next = move->next;
			}
			kfree(move);
			mutex_unlock(&tree_lock);
			return ;
		}
		else
			head = move;
	}
	mutex_unlock(&tree_lock);
}

// white list
/*
int pkgl_partial_search(const char *name, int check_connection)
{
	struct pkg_list *move;
	struct timespec tp;

	// if uptime < 5mins, we allow all apps to access network
	ktime_get_ts(&tp);
	monotonic_to_bootbased(&tp);
	if (tp.tv_sec < DELAY_300_SECS)
		return 1;

	if (!strcmp(name, "android"))
		return 0;

	mutex_lock(&tree_lock);
	for (move = pkgl; move != NULL; move = move->next) {
		if (check_connection == 0) {
			if (strstr(move->name, name)) {
				mutex_unlock(&tree_lock);
				return 1;
			}
		}
		else if (check_connection == 1) {
			if (strstr(move->name, name) && move->block_connection == 0) {
				mutex_unlock(&tree_lock);
				return 1;
			}
		}
	}
	mutex_unlock(&tree_lock);
	return 0;
}
*/

// black list
int pkgl_partial_search(const char *uid, int check_connection)
{
	struct pkg_list *move;
	unsigned long long t;
	unsigned long nanosec_rem;

	// if uptime < 5mins, we allow all apps to access network
	t = cpu_clock(0);
	nanosec_rem = do_div(t, 1000000000);
	if (t < DELAY_300_SECS)
		return 0;

	mutex_lock(&tree_lock);
	for (move = pkgl; move != NULL; move = move->next) {
		if (strstr(move->uid, uid)) {
			mutex_unlock(&tree_lock);
			return 1;
		}
	}
	mutex_unlock(&tree_lock);
	return 0;
}

ssize_t black_list_added_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	char *s = buf;
	char *end = buf + PAGE_SIZE;
	struct pkg_list *move;

	mutex_lock(&tree_lock);
	for (move = pkgl; move != NULL; move = move->next) {
		s += scnprintf(s, end - s, "%s[%d]\n", move->uid, move->disconnected);
	}
	s += scnprintf(s, end - s, "\n");

	mutex_unlock(&tree_lock);
	return (s - buf);
}

ssize_t black_list_added_store(struct kobject *kobj, struct kobj_attribute *attr,
			const char *buf, size_t n)
{
	const char *arg;
	int name_len = 0;
	char name[NAME_LEN];

	arg = buf;
	while (*arg && !isspace(*arg))
		arg++;
	name_len = arg - buf;
	if (!name_len)
		goto bad_name;
	if (name_len >= NAME_LEN)
		name_len = NAME_LEN - 1;

	strncpy(name, buf, name_len);
	name[name_len] = 0;
	pkgl_add(name, 0);

bad_name:

	return n;
}

ssize_t  black_list_removed_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	return black_list_added_show(kobj, attr, buf);
}

ssize_t  black_list_removed_store(struct kobject *kobj, struct kobj_attribute *attr,
			const char *buf, size_t n)
{
	const char *arg;
	int name_len = 0;
	char name[NAME_LEN];

	arg = buf;
	while (*arg && !isspace(*arg))
		arg++;
	name_len = arg - buf;
	if (!name_len)
		goto bad_name;
	if (name_len >= NAME_LEN)
		name_len = NAME_LEN - 1;

	strncpy(name, buf, name_len);
	name[name_len] = 0;

	pkgl_remove(name);

bad_name:

	return n;
}

static int atoi(const char *name)
{
	int val = 0;

	for (;; name++) {
		switch (*name) {
		case '0' ... '9':
			val = 10*val+(*name-'0');
			break;
		case 'a' ... 'z':
		case 'A' ... 'Z':
			val = 0;
			break;
		default:
			return val;
		}
	}
}

ssize_t  black_list_search_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	char *s = buf;
	char *end = buf + PAGE_SIZE;
	struct task_struct *parent;
	struct task_struct *who;
	int pid;

	if (blacklist_debug)
		pr_info("[BlackList] name=%s\r\n", search_name);
	if (search_name[0] == 0) {
		search_pid = 0;
		goto return_zero;
	}

	if (search_pid != current->pid) {
		goto return_zero;
	}

	if (0 && (pid = atoi(search_name)) > 0) {  // for pid number
		who = find_task_by_vpid(pid);
		if (who == NULL || who->nsproxy == NULL) {
			goto clean_name;
		}

		parent = find_task_by_vpid(who->tgid);
		if (!parent) {
			goto clean_name;
		}
		strcpy(search_name, parent->comm);
	}
	else if (strstr(search_name, "search_myself")) {  // for current thread
		if (current == NULL || current->nsproxy == NULL) {
			goto clean_name;
		}

		parent = find_task_by_vpid(current->tgid);
		if (!parent) {
			goto clean_name;
		}
		strcpy(search_name, parent->comm);
	}

	if (pkgl_partial_search(search_name, 1)) {
		if (blacklist_debug)
			pr_info("[BlackList] name=%s   black!!!!\r\n", search_name);
		search_name[0] = 0;
		search_pid = 0;
		s += scnprintf(s, end - s, "1");
		return (s - buf);
	}

clean_name:
	search_name[0] = 0;
	search_pid = 0;

return_zero:
	s += scnprintf(s, end - s, "0");
	return (s - buf);
}

ssize_t  black_list_search_store(struct kobject *kobj, struct kobj_attribute *attr,
			const char *buf, size_t n)
{
	const char *arg;
	int name_len = 0;
	unsigned long long t;
	unsigned long nanosec_rem;

	arg = buf;
	while (*arg && !isspace(*arg))
		arg++;
	name_len = arg - buf;
	if (!name_len)
		goto bad_name;
	if (name_len >= NAME_LEN)
		name_len = NAME_LEN - 1;

	if (search_pid == 0)
		search_pid = current->pid;
	else {
		t = cpu_clock(0);
		nanosec_rem = do_div(t, 1000000000);
		while(search_pid && (t - search_pid_time < 3))
			msleep(300);
		search_pid = current->pid;
	}
	t = cpu_clock(0);
	nanosec_rem = do_div(t, 1000000000);
	search_pid_time = t;

	strncpy(search_name, buf, name_len);
	search_name[name_len] = 0;

bad_name:
	return n;
}

static int __init black_list_init(void)
{
	int i;

	printk(KERN_INFO "black_list_init()\n");
	for (i = 0; i < ARRAY_SIZE(default_ap_list); i++) {
		if (default_ap_list[i].uid[0])
			pkgl_add(default_ap_list[i].uid, default_ap_list[i].disconnected);
	}

	return 0;
}

module_init(black_list_init);
