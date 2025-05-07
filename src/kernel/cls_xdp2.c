// SPDX-License-Identifier: BSD-2-Clause-FreeBSD
/*
 * Copyright (c) 2025 Tom Herbert
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <linux/module.h>
#include <linux/slab.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/rtnetlink.h>
#include <linux/skbuff.h>
#include <linux/idr.h>
#include <net/netlink.h>
#include <net/act_api.h>
#include <net/pkt_cls.h>

#include "kernel/cls_xdp2.h"

struct xdp2_head {
	struct tc_cls_xdp2_ops *ops;
	struct list_head filters;
	struct idr handle_idr;
	struct rcu_head rcu;
};

struct xdp2_filter {
	u32 handle;
	struct list_head list;
	struct tcf_result res;
	struct rcu_work rwork;
};

static LIST_HEAD(ops_list);
static DEFINE_RWLOCK(ops_mod_lock);

static struct tc_cls_xdp2_ops *xdp2_find_ops(const char *name)
{
	struct tc_cls_xdp2_ops *cursor;

	read_lock(&ops_mod_lock);
	list_for_each_entry(cursor, &ops_list, list) {
		if (strcmp(cursor->name, name) == 0) {
			if (!try_module_get(cursor->owner))
				cursor = NULL;
			read_unlock(&ops_mod_lock);
			return cursor;
		}
	}
	read_unlock(&ops_mod_lock);

	return NULL;
}

int register_xdp2_ops(struct tc_cls_xdp2_ops *ops)
{
	struct tc_cls_xdp2_ops *cursor;
	int err = 0;

	if (!ops->name || !ops->parse)
		return -EINVAL;

	write_lock(&ops_mod_lock);

	list_for_each_entry(cursor, &ops_list, list) {
		if (strcmp(cursor->name, ops->name) == 0) {
			err = -EEXIST;
			goto out;
		}
	}

	INIT_LIST_HEAD(&ops->list);
	list_add(&ops->list, &ops_list);
out:
	write_unlock(&ops_mod_lock);
	return err;
}
EXPORT_SYMBOL_GPL(register_xdp2_ops);

int unregister_xdp2_ops(struct tc_cls_xdp2_ops *ops)
{
	struct tc_cls_xdp2_ops *cursor;
	int err = -ENOENT;

	write_lock(&ops_mod_lock);
	list_for_each_entry(cursor, &ops_list, list) {
		if (strcmp(cursor->name, ops->name) == 0) {
			list_del(&ops->list);
			err = 0;
			break;
		}
	}
	write_unlock(&ops_mod_lock);

	return err;
}
EXPORT_SYMBOL_GPL(unregister_xdp2_ops);

static const struct nla_policy xdp2_policy[TCA_XDP2_MAX + 1] = {
	[TCA_XDP2_CLASSID] = { .type = NLA_U32 },
	[TCA_XDP2_PARSER] = { .type = NLA_NUL_STRING, .len = 255 },
};

static int xdp2_init(struct tcf_proto *tp)
{
	struct xdp2_head *head;

	head = kzalloc(sizeof(*head), GFP_KERNEL);
	if (head == NULL)
		return -ENOBUFS;
	INIT_LIST_HEAD(&head->filters);
	idr_init(&head->handle_idr);
	rcu_assign_pointer(tp->root, head);

	return 0;
}

static int xdp2_classify(struct sk_buff *skb, const struct tcf_proto *tp,
			  struct tcf_result *res)
{
	struct xdp2_head *head = rcu_dereference_bh(tp->root);
	struct xdp2_filter *filter;

	list_for_each_entry_rcu(filter, &head->filters, list) {
		int ret = head->ops->parse(skb);

		if (ret < 0)
			continue;
		/* TODO: return something else */
		return 0;
	}

	return 0;
}

static void *xdp2_get(struct tcf_proto *tp, u32 handle)
{
	struct xdp2_head *head = rtnl_dereference(tp->root);
	struct xdp2_filter *filter;

	list_for_each_entry(filter, &head->filters, list) {
		if (filter->handle == handle)
			return filter;
	}

	return NULL;
}

static void xdp2_delete_filter_work(struct work_struct *work)
{
	struct xdp2_filter *filter =
		container_of(to_rcu_work(work), struct xdp2_filter, rwork);
	rtnl_lock();
	kfree(filter);
	rtnl_unlock();
}

static void xdp2_destroy(struct tcf_proto *tp, bool rtnl_held,
			  struct netlink_ext_ack *extack)
{
	struct xdp2_head *head = rtnl_dereference(tp->root);
	struct xdp2_filter *filter, *tmp;

	list_for_each_entry_safe(filter, tmp, &head->filters, list) {
		list_del_rcu(&filter->list);
		tcf_unbind_filter(tp, &filter->res);
		idr_remove(&head->handle_idr, filter->handle);
		kfree(filter);
		module_put(head->ops->owner);
	}
	idr_destroy(&head->handle_idr);
	kfree_rcu(head, rcu);
}

static int xdp2_delete(struct tcf_proto *tp, void *arg, bool *last,
			bool rtnl_held, struct netlink_ext_ack *extack)
{
	struct xdp2_head *head = rtnl_dereference(tp->root);
	struct xdp2_filter *filter = arg;

	list_del_rcu(&filter->list);
	tcf_unbind_filter(tp, &filter->res);

	idr_remove(&head->handle_idr, filter->handle);
	module_put(head->ops->owner);

	*last = list_empty(&head->filters);

	tcf_queue_work(&filter->rwork, xdp2_delete_filter_work);

	return 0;
}

static int xdp2_set_params(struct tcf_proto *tp, struct nlattr **tb,
			    struct xdp2_head *head,
			    struct xdp2_filter *filter, unsigned long base,
			    bool rtnl_held)
{
	const char *name = nla_memdup(tb[TCA_XDP2_PARSER], GFP_KERNEL);
	struct tc_cls_xdp2_ops *ops;
	int ret = 0;

	if (!name)
		return -ENOMEM;

	if (!head->ops) {
		ops = xdp2_find_ops(name);
		if (!ops) {
#if IS_ENABLED(CONFIG_MODULES)
			if (rtnl_held)
				rtnl_unlock();
			request_module("xdp2_%s", name);
			if (rtnl_held)
				rtnl_lock();
			ops = xdp2_find_ops(name);
#endif
		}

		if (!ops) {
			ret = -ENOENT;
			goto out;
		}

		head->ops = ops;
	} else if (strcmp(head->ops->name, name) != 0) {
		/* Loading a different parser in a handle is invalid */
		ret = -EINVAL;
		goto out;
	}

	if (tb[TCA_XDP2_CLASSID]) {
		filter->res.classid = nla_get_u32(tb[TCA_XDP2_CLASSID]);
		tcf_bind_filter(tp, &filter->res, base);
	}

out:
	kfree(name);
	return ret;
}

static int xdp2_change(struct net *net, struct sk_buff *in_skb,
			struct tcf_proto *tp, unsigned long base, u32 handle,
			struct nlattr **tca, void **arg, unsigned int flags,
			struct netlink_ext_ack *extack)
{
	struct xdp2_head *head = rtnl_dereference(tp->root);
	struct xdp2_filter *fold = *arg;
	struct xdp2_filter *fnew;
	struct nlattr *tb[TCA_XDP2_MAX + 1];
	int err;

	if (tca[TCA_OPTIONS] == NULL)
		return -EINVAL;

	err = nla_parse_nested_deprecated(tb, TCA_XDP2_MAX, tca[TCA_OPTIONS],
					  xdp2_policy, NULL);
	if (err < 0)
		return err;

	if (!tb[TCA_XDP2_PARSER])
		return -EINVAL;

	if (fold != NULL) {
		if (handle && fold->handle != handle)
			return -EINVAL;
	}

	fnew = kzalloc(sizeof(*fnew), GFP_KERNEL);
	if (!fnew)
		return -ENOBUFS;

	if (!handle) {
		handle = 1;
		err = idr_alloc_u32(&head->handle_idr, fnew, &handle, INT_MAX,
				    GFP_KERNEL);
	} else if (!fold) {
		err = idr_alloc_u32(&head->handle_idr, fnew, &handle, handle,
				    GFP_KERNEL);
	}
	if (err)
		goto err;

	fnew->handle = handle;

	err = xdp2_set_params(tp, tb, head, fnew, base,
			      !(flags & TCA_ACT_FLAGS_NO_RTNL));
	if (err < 0) {
		if (!fold)
			idr_remove(&head->handle_idr, fnew->handle);
		goto err;
	}

	*arg = fnew;

	if (fold) {
		idr_replace(&head->handle_idr, fnew, fnew->handle);
		list_replace_rcu(&fold->list, &fnew->list);
		tcf_unbind_filter(tp, &fold->res);
		tcf_queue_work(&fold->rwork, xdp2_delete_filter_work);
	} else {
		list_add_rcu(&fnew->list, &head->filters);
	}

	return 0;
err:
	kfree(fnew);
	return err;
}

static void xdp2_walk(struct tcf_proto *tp, struct tcf_walker *arg,
		       bool rtnl_held)
{
	struct xdp2_head *head = rtnl_dereference(tp->root);
	struct xdp2_filter *filter;

	list_for_each_entry(filter, &head->filters, list) {
		if (arg->count < arg->skip)
			goto skip;

		if (arg->fn(tp, filter, arg) < 0) {
			arg->stop = 1;
			break;
		}
skip:
		arg->count++;
	}
}

static void xdp2_bind_class(void *fh, u32 classid, unsigned long cl, void *q,
			     unsigned long base)
{
	struct xdp2_filter *filter = fh;

	if (filter && filter->res.classid == classid) {
		if (cl)
			__tcf_bind_filter(q, &filter->res, base);
		else
			__tcf_unbind_filter(q, &filter->res);
	}
}

static int xdp2_dump(struct net *net, struct tcf_proto *tp, void *fh,
		      struct sk_buff *skb, struct tcmsg *t, bool rtnl_held)
{
	struct xdp2_head *head = rtnl_dereference(tp->root);
	struct xdp2_filter *filter = fh;
	struct nlattr *nest;

	if (filter == NULL)
		return skb->len;

	t->tcm_handle = filter->handle;

	nest = nla_nest_start_noflag(skb, TCA_OPTIONS);
	if (nest == NULL)
		goto nla_put_failure;

	if (filter->res.classid &&
	    nla_put_u32(skb, TCA_XDP2_CLASSID, filter->res.classid))
		goto nla_put_failure;

	if (nla_put_string(skb, TCA_XDP2_PARSER, head->ops->name))
		goto nla_put_failure;

	nla_nest_end(skb, nest);

	return skb->len;

nla_put_failure:
	nla_nest_cancel(skb, nest);
	return -EMSGSIZE;
}

static struct tcf_proto_ops cls_basic_ops __read_mostly = {
	.kind = "xdp2",
	.classify = xdp2_classify,
	.init = xdp2_init,
	.destroy = xdp2_destroy,
	.get = xdp2_get,
	.change = xdp2_change,
	.delete = xdp2_delete,
	.walk = xdp2_walk,
	.dump = xdp2_dump,
	.bind_class = xdp2_bind_class,
	.owner = THIS_MODULE,
};

static int __init init_xdp2(void)
{
	return register_tcf_proto_ops(&cls_basic_ops);
}

static void __exit exit_xdp2(void)
{
	unregister_tcf_proto_ops(&cls_basic_ops);
}

module_init(init_xdp2);
module_exit(exit_xdp2);
MODULE_AUTHOR("Pedro Tammela <pctammela@mojatatu.com>");
MODULE_AUTHOR("Tom Herbert");
MODULE_LICENSE("GPL");
