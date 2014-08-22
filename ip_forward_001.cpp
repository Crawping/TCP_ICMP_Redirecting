// ip_forward_001.cpp : �������̨Ӧ�ó������ڵ㡣
//By Dexter Ju
//jvdajd@gmail.com
//TCP and ICMP packet redirecting.
//conf.txt
#include "stdafx.h"
#include <winsock2.h>
#include <Windows.h>
#include <ws2tcpip.h>
#include <stdio.h>
#include <mstcpip.h>
#include "struct.h"
#include <pcap.h>
#include "ip_address.h"
#include <thread>
#include <string>
#include <iostream>
#include<fstream>
#pragma comment(lib, "Ws2_32.lib")

#define BUFF_SIZE 65535//�����С
#define TYPE_IP 0X0800


char errbuf[PCAP_ERRBUF_SIZE];
int int_num = 0;
u_char gateway_mac[6] ;
ip_pkt * ip_header = NULL;
u_int netmask_source;
u_int netmask_destination;
u_int ip_source = NULL;//Դ��������ip��ַ
u_int ip_destination = NULL;//Ŀ�ĵ�������IP��ַ
u_int ip_victim = NULL;//�ܺ�������IP
u_int ip_gateway = NULL;//ת������������IP

const char * packet_filter_s =NULL;
const char * packet_filter_d =NULL;
struct bpf_program fcode_s;//Դ�˹�����
struct bpf_program fcode_d;//Ŀ�ĵض˹�����

pcap_addr_t *a;
pcap_t * source_winpcap = NULL;//source���
pcap_t * destination_winpcap = NULL;//destination���
pcap_if_t *alldevs;//������ʾ�豸�б��ָ��
pcap_if_t *d;//������ʾ�豸�б��ָ��

in_addr destination_in_addr;
u_char source_mac[6] = { NULL };//Դ��������MAC��ַ
u_char destination_mac[6] = { NULL };//Ŀ�Ķ�������MAC��ַ
u_char from_mac[6] = { NULL };//Դ����Դ��mac��ַ

const char * target = NULL; //αװ��Ŀ��վ��
std::string target_s;
std::string redriect_address;
char internet_gateway[15] = { NULL };//���������ص�ַ
char ifile_buff[4][100] = { NULL };

u_char source_buff[BUFF_SIZE] = { NULL };//Դ�˻���
u_char destination_buff[BUFF_SIZE] = { NULL };//Ŀ�Ķ˻���
u_long victim_address = { NULL };//�ܺ���IP��ַ


std::ifstream ifile;

int inum;
int i = 0;

//��ʼ������MAC��ַ




void source_handler(u_char *param, const struct pcap_pkthdr *header, const u_char *pkt_data);
void destination_handler(u_char *param, const struct pcap_pkthdr *header, const u_char *pkt_data);

void source_fun(){
	printf("��������������Ѿ�������\n");
	pcap_loop(source_winpcap, 0, (pcap_handler)source_handler, NULL);
}
void destination_fun(){
	printf("�������������2���Ѿ�������\n");
	pcap_loop(destination_winpcap, 0, (pcap_handler)destination_handler, NULL);
}
void source_handler(u_char *param, const struct pcap_pkthdr *header, const u_char *pkt_data) //Դ�˵Ļص�����
{
	//�����Դ�˷�����TCP������������ת����������
	et_header *eth_ptr;
	ip_pkt * ip_ptr;
	eth_ptr = (et_header *)pkt_data;
	ip_ptr = (ip_pkt *)(pkt_data+ETH_HEADER);//���target����IPɸѡ
		char * temp = inet_ntoa(ip_ptr->src);
		if (fliter_ip(ip_ptr->src.s_addr,ip_destination))
		{
			//��֤�Ƿ��Ǳ��������İ������IP��Ƚ��Ϊ��
			
			return;

		}
#ifndef FIRSTPACKET
#define FIRSTPACKET
		memcpy(from_mac, eth_ptr->eh_src, 6);//��¼��Դ���ص�MAC��ַ
		
#endif


		victim_address = ip_ptr->src.s_addr;//ץȡ�ܺ��ߵ�ַ
		memcpy(source_buff, pkt_data, header->len);
		eth_ptr = (et_header*)source_buff;
		ip_ptr = (ip_pkt *)(source_buff + ETH_HEADER);//��ʼ���
		memcpy(eth_ptr->eh_dst, gateway_mac, 6);//��д������MAC���ص�ַ
		memcpy(eth_ptr->eh_src, destination_mac, 6);
		eth_ptr->eh_type = htons(TYPE_IP);
		ip_ptr->src.s_addr = ip_destination;//����������������
		const char * redriect_addr = redriect_address.c_str();
		ip_ptr->dst.s_addr = inet_addr(redriect_addr);//Ŀ���Ϊ�ض���IP
		ip_ptr->cksum = 0;
		ip_ptr->cksum = in_cksum((u_short *)ip_ptr, 20);
		
		switch (ip_ptr->pro)
		{
		case IPPROTO_TCP:

			//���TCPУ���
			tcp_cksum(source_buff);
			break;
		case IPPROTO_ICMP:
			icmp_cksum(source_buff, header->len);
			break;
		default:
			return;
		}
		if ((pcap_sendpacket(destination_winpcap, (u_char*)source_buff, header->len)) != 0)
		{
			fprintf(stderr, "\nError sending the packet : \n", pcap_geterr(destination_winpcap));
			return;
		}

		memset(source_buff, NULL, header->len);
		printf("Դ���������ݰ�ת���ɹ�������Ϊ��%d\n", header->len);
		return;

}

void destination_handler(u_char *param, const struct pcap_pkthdr *header, const u_char *pkt_data)//Ŀ�ĵض˵Ļص�����
{
	//��������������ص�TCP������������д������ظ�Դ��
	et_header *eth_ptr;
	ip_pkt * ip_ptr;
	eth_ptr = (et_header *)pkt_data;
	ip_ptr = (ip_pkt *)(pkt_data + ETH_HEADER);//���target����IPɸѡ
//ȷ�������ض���Ŀ�귵�صİ�
	
		if (fliter_mac(eth_ptr->eh_src, source_mac))
		{
			//ip�Ѿ���дΪtargetIP����Ҫ��MAC��ַ����ɸѡ,��ֹ��׽����source������α��������sourcemac�Ƿ�Ϊ���������Ϊ��������
			return;

		}
		memcpy(destination_buff, pkt_data, header->len);
		eth_ptr = (et_header*)destination_buff;
		ip_ptr = (ip_pkt *)(destination_buff + ETH_HEADER);//��ʼ���
		memcpy(eth_ptr->eh_dst, from_mac, 6);//��д����MAC���ص�ַ
		memcpy(eth_ptr->eh_src, source_mac, 6);
		eth_ptr->eh_type = htons(TYPE_IP);
		ip_ptr->src.s_addr = inet_addr(target);//��дĿ����վIP
		ip_ptr->dst.s_addr = victim_address;//��д�ܺ���IP
		ip_ptr->cksum = 0;
		ip_ptr->cksum = in_cksum((u_short *)ip_ptr, 20);
		//���TCPУ���
		switch (ip_ptr->pro)
		{
		case IPPROTO_TCP:

			//���TCPУ���
			tcp_cksum(destination_buff);
			break;
		case IPPROTO_ICMP:
			
			icmp_cksum(destination_buff, header->len);
			break;
		default:
			return;
		}
		
		if ((pcap_sendpacket(source_winpcap, (u_char*)destination_buff, header->len)) != 0)
		{
			fprintf(stderr, "\nError sending the packet : \n", pcap_geterr(destination_winpcap));
			return;
		}

		memset(destination_buff, NULL, header->len);
		printf("Ŀ�ĵض��������ݰ�ת���ɹ�������Ϊ��%d\n", header->len);
		return;

}






int _tmain(int argc, _TCHAR* argv[])
{
	//��ȡ�����ļ�
	ifile.open("c:/conf.txt",std::ios::in);

	std::string config;
	int index = 0;
	while (ifile.getline(ifile_buff[index], 100)){
		index++;
	}
	packet_filter_s = ifile_buff[0];
	packet_filter_d = ifile_buff[1];
	target_s = ifile_buff[2];
	redriect_address = ifile_buff[3];
	
	//���������豸������
	loadiphlpapi();
	//����������ѡ��ͳ�ʼ��
	if (pcap_findalldevs(&alldevs, errbuf) == -1)
	{
		fprintf(stderr, "Error in pcap_findalldevs: %s\n", errbuf);
		return -1;
	}


	//��ӡ�豸�б�
	for (d = alldevs; d; d = d->next)
	{
		ifprint(d, i);
		++i;
	}
	int_num = i;

	if (i == 0)
	{
		printf("\nNo interfaces found! Make sure WinPcap is installed.\n");
		return -1;
	}

	printf("Enter the interface number for source (1-%d):", int_num);//ѡ��Դ�������豸
	scanf("%d", &inum);

	if (inum < 1 || inum > i)
	{
		printf("\nInterface number out of range.\n");
		//�ͷ��豸�б�
		pcap_freealldevs(alldevs);
		return -1;
	}

	//��ת����ѡ�е��豸
	for (d = alldevs, i = 0; i < inum - 1; d = d->next, i++);
	for (a = d->addresses; a; a = a->next) {
		switch (a->addr->sa_family)
		{
		case AF_INET:
			printf("\tAddress Family Name: AF_INET\n");
			if (a->addr)
				ip_source = ((struct sockaddr_in *)a->addr)->sin_addr.s_addr;
			if (a->netmask)
				netmask_source = ((struct sockaddr_in *)a->netmask)->sin_addr.s_addr;
			break;
		default:
			continue;
		}
	}
	if (get_mac_address(d, source_mac))
	{
		printf("��ȡMAC��ַʧ�ܣ�");
		return -1;
	}
	/* Open the adapter */
	if ((source_winpcap = pcap_open_live(d->name,	// name of the device
		65536,			// portion of the packet to capture. 
		// 65536 grants that the whole packet will be captured on all the MACs.
		1,				// promiscuous mode (nonzero means promiscuous)
		10,			// read timeout
		errbuf			// error buffer
		)) == NULL)
	{
		fprintf(stderr, "\nUnable to open the adapter. %s is not supported by WinPcap\n", d->name);
		/* Free the device list */
		pcap_freealldevs(alldevs);
		return -1;
	}
	//���������
	if (pcap_compile(source_winpcap, &fcode_s, packet_filter_s, 1, netmask_source) <0)
	{
		fprintf(stderr, "\nUnable to compile the packet filter. Check the syntax.\n");
		/* �ͷ��豸�б� */
		pcap_freealldevs(alldevs);
		return -1;
	}

	//���ù�����
	if (pcap_setfilter(source_winpcap, &fcode_s)<0)
	{
		fprintf(stderr, "\nError setting the filter.\n");
		/* �ͷ��豸�б� */
		pcap_freealldevs(alldevs);
		return -1;
	}


	//����Ŀ�ĵ��豸�Ĵ���
	//destination
	//��Ҫ��ӻ�ȡ����IP��mac��ַ�Ĺ���
	printf("Enter the interface number for destination (1-%d):", int_num);//ѡ��Ŀ�ĵ��豸
	scanf("%d", &inum);
	if (inum < 1 || inum > int_num)
	{
		printf("\nInterface number out of range.\n");
		/* Free the device list */
		pcap_freealldevs(alldevs);
		return -1;
	}

	/* Jump to the selected adapter */
	for (d = alldevs, i = 0; i < inum - 1; d = d->next, i++);
		for (a = d->addresses; a; a = a->next) {
			switch (a->addr->sa_family)
			{
			case AF_INET:
				printf("\tAddress Family Name: AF_INET\n");
				if (a->addr)
					destination_in_addr = ((struct sockaddr_in *)a->addr)->sin_addr;
					ip_destination = ((struct sockaddr_in *)a->addr)->sin_addr.s_addr;//��ȡ����IP
				if (a->netmask)
					netmask_destination = ((struct sockaddr_in *)a->netmask)->sin_addr.s_addr;//��ȡ��������
				break;
			default:
				continue;
			}
	}
	
	if (get_mac_address(d,destination_mac))
	{
		printf("��ȡMAC��ַʧ�ܣ�");
		return -1;
	}
	/* Open the adapter */
	if ((destination_winpcap = pcap_open_live(d->name,	// name of the device
		65536,			// portion of the packet to capture. 
		// 65536 grants that the whole packet will be captured on all the MACs.
		1,				// promiscuous mode (nonzero means promiscuous)
		10,			// read timeout
		errbuf			// error buffer
		)) == NULL)
	{
		fprintf(stderr, "\nUnable to open the adapter. %s is not supported by WinPcap\n", d->name);
		/* Free the device list */
		pcap_freealldevs(alldevs);
		return -1;
	}
	//׼����ȡ����IP��mac��ַ

	get_gateway(destination_in_addr, internet_gateway);
	printf("��ȡ�������ص�ַΪ%s\n", internet_gateway);
	ip_gateway = inet_addr(internet_gateway);
	get_gateway_mac_address(gateway_mac, ip_gateway);
	printf("��ȡ�������豸��MAC��ַΪ %.2x:%.2x:%.2x:%.2x:%.2x:%.2x\n",
		gateway_mac[0],
		gateway_mac[1],
		gateway_mac[2],
		gateway_mac[3],
		gateway_mac[4],
		gateway_mac[5]);


	if (pcap_compile(destination_winpcap, &fcode_d, packet_filter_d, 1, netmask_destination) <0)
	{
		fprintf(stderr, "\nUnable to compile the packet filter. Check the syntax.\n");
		/* �ͷ��豸�б� */
		pcap_freealldevs(alldevs);
		return -1;
	}

	//���ù�����
	if (pcap_setfilter(destination_winpcap, &fcode_d)<0)
	{
		fprintf(stderr, "\nError setting the filter.\n");
		/* �ͷ��豸�б� */
		pcap_freealldevs(alldevs);
		return -1;
	}
	//printf("�������ض����Ŀ��IP��\n");
	//fflush(stdin);//��ջ�����
	//std::cin>>redriect_address;
	//fflush(stdin);//��ջ�����
	//printf("��������Ҫαװ��IP��ַ��\n");
	//std::cin >> target_s;
	target = target_s.c_str();
	std::thread source(source_fun);
	std::thread destination(destination_fun);
	source.join();
	destination.join();


	return 0;
}


