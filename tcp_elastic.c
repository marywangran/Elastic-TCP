#include <linux/module.h>
#include <net/tcp.h>

struct elastic {
	u32 ai;
	u32 rtt_max;
	u32 rtt_current;
};

static void elastic_init(struct sock *sk)
{
	struct elastic *ca = inet_csk_ca(sk);

	ca->ai = 0;
	ca->rtt_max = 0;
	ca->rtt_current = 1;
}

static void elastic_cong_avoid(struct sock *sk, u32 ack, u32 acked)
{
	struct tcp_sock *tp = tcp_sk(sk);
	struct elastic *ca = inet_csk_ca(sk);

	if (!tcp_is_cwnd_limited(sk))
		return;

	if (tcp_in_slow_start(tp))
		tcp_slow_start(tp, acked);
	else {
		tp->snd_cwnd_cnt +=
		    int_sqrt(ca->rtt_max / ca->rtt_current * tp->snd_cwnd) /
		    tp->snd_cwnd;
		if (tp->snd_cwnd_cnt >= tp->snd_cwnd) {
			tp->snd_cwnd_cnt = 0;
			tp->snd_cwnd++;
		}
	}
}

static void elastic_rtt_calc(struct sock *sk, const struct ack_sample *sample)
{
	struct elastic *ca = inet_csk_ca(sk);
	u32 rtt, rtt_max;

	rtt = sample->rtt_us + 1;

	rtt_max = ca->rtt_max;

	ca->rtt_current = rtt;

	if (rtt > rtt_max || rtt_max == 0)
		rtt_max = rtt;

	ca->rtt_max = rtt_max;
}

static void tcp_elastic_event(struct sock *sk, enum tcp_ca_event event)
{
	struct elastic *ca = inet_csk_ca(sk);

	switch (event) {
	case CA_EVENT_LOSS:
		ca->rtt_max = 0;
	default:
		/* don't care */
		break;
	}
}

static struct tcp_congestion_ops tcp_elastic __read_mostly = {
    .init = elastic_init,
    .ssthresh = tcp_reno_ssthresh,
    .undo_cwnd = tcp_reno_undo_cwnd,
    .cong_avoid = elastic_cong_avoid,
    .pkts_acked = elastic_rtt_calc,
    .cwnd_event = tcp_elastic_event,
    .owner = THIS_MODULE,
    .name = "elastic"};

static int __init elastic_register(void)
{
	BUILD_BUG_ON(sizeof(struct elastic) > ICSK_CA_PRIV_SIZE);
	return tcp_register_congestion_control(&tcp_elastic);
}

static void __exit elastic_unregister(void)
{
	tcp_unregister_congestion_control(&tcp_elastic);
}

module_init(elastic_register);
module_exit(elastic_unregister);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Elastic TCP");
