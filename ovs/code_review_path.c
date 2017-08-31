=======数据流=======
ovs_vport_receive()   --- [vport.c]
    skb->dev != vport->dp => skb_scrub_packet(skb, true);  drop
    ovs_flow_key_extract(tun_info, skb, &key)   提取各层报文头信息存于sw_flow_key
        key->phy
        key_extract_mac_proto(skb);    key->mac_proto
        key_extract(skb, key);
            skb_reset_mac_header(skb);   skb->mac_header = skb->data - skb->head;  skb_pull/skb_push后做的，重新复位sk_buf网络头部地址
            eth_hdr()
            __skb_pull(skb, 2 * ETH_ALEN);
            skb_reset_network_header(skb);   skb->network_header = skb->data - skb->head;
            __skb_push(skb, skb->data - skb_mac_header(skb));
            key->eth.type = skb->protocol;
            if (key->eth.type == htons(ETH_P_IP)):
                nh = ip_hdr(skb);
                key->ip.proto = nh->protocol;
                if (key->ip.proto == IPPROTO_TCP):
                    struct tcphdr *tcp = tcp_hdr(skb)   key->tp.src = tcp->source;  tcp/udp统一
                if (key->ip.proto == IPPROTO_ICMP)
                    struct icmphdr *icmp = icmp_hdr(skb)   key->tp.src = htons(icmp->type) key->tp.dst = htons(icmp->code)
            if key->eth.type ETH_P_ARP  eth_p_mpls  ETH_P_IPV6
        ovs_ct_fill_key()   链接跟踪信息
    ovs_dp_process_packet(skb, &key)   --- [datapath.c]
        flow = ovs_flow_tbl_lookup_stats(&dp->table, key, skb_get_hash(skb), &n_mask_hit);  --- [flow_table.c]
            flow_lookup(tbl, ti, ma, key, n_mask_hit, &mask_index);
                有缓存机制 根据flow的各特征值，mask hash快速查找
                masked_flow_lookup(ti, key, mask, n_mask_hit);
        if (unlikely(!flow)):    未命中flow，走upcall流程
            ovs_dp_upcall(dp, skb, key, &upcall, 0);  
            ===========
        else sf_acts = rcu_dereference(flow->sf_acts);  命中提取action
            ovs_execute_actions(dp, skb, sf_acts, key);  --- [action.c]
            do_execute_actions(dp, skb, key, sf_acts->actions, sf_acts->actions_len);
            ===========
        