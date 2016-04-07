#include "udp.hpp"

// class udp_rule
udp_rule::udp_rule()
	: ip_header_r(6), ip_rule() {}
udp_rule::udp_rule(std::vector<std::string> tkn_rule)
	: ip_header_r(6), ip_rule(tkn_rule) {}
void udp_rule::parse(boost::program_options::options_description& opt)
{
	parser::command_parser cp(opt);
	boost::program_options::variables_map vm = cp.parse(tokenize_rule);
	// store text rule
	text_rule = cp.join(tokenize_rule);
	// parse L3 header
	ip_header_parse(vm);
	// parse rule options
	ip_rule_parse(vm);
	// parse L4 header
	if (vm.count("sport")) {
		src_port = parser::range_from_port_string(vm["sport"].as<std::string>());
	}
	if (vm.count("dport")) {
		dst_port = parser::range_from_port_string(vm["dport"].as<std::string>());
	}
	if (vm.count("hlen")) {
		len = parser::numcomp_from_string<uint16_t>(vm["hlen"].as<std::string>());
	}
}
bool udp_rule::check_packet(struct udphdr *udp_hdr, uint32_t s_addr, uint32_t d_addr) const
{
	// L3 header check
	if(!ip_src.in_this(s_addr)) // check source ip address
		return false;
	if(!ip_dst.in_this(d_addr)) // check destination ip address
		return false;
	// L4 header check
	uint16_t h_sport = ntohs(udp_hdr->source);
	if(!src_port.in_this(h_sport))
		return false;
	uint16_t h_dport = ntohs(udp_hdr->dest);
	if(!dst_port.in_this(h_dport))
		return false;
	uint16_t h_len = udp_hdr->len;
	if(!len.in_this(h_len))
		return false;

	// std::cout << "\n\n== IP HEADER ==";
	// std::cout << "\nSource IP: " << boost::asio::ip::address_v4(s_addr).to_string();
	// std::cout << "\nDestination IP: " << boost::asio::ip::address_v4(d_addr).to_string();
	// // TCP Header
	// std::cout << "\n== UDP HEADER ==";
	// std::cout << "\nSource Port: " << std::dec << h_sport;
	// std::cout << "\nDestination Port: " << std::dec << h_dport;
	// std::cout << "\nHeader lenght: " << h_len;
	// std::cout << "\nChecksum: " << std::hex << ntohs(udp_hdr->check);

	return true;
}
bool udp_rule::operator==(udp_rule const & other) const
{
	return (src_port == other.src_port
		&& dst_port == other.dst_port
		&& ip_src == other.ip_src
		&& ip_dst == other.ip_dst
		&& next_rule == other.next_rule
		&& pps_trigger == other.pps_trigger
		&& bps_trigger == other.bps_trigger
		&& pps_trigger_period == other.pps_trigger_period
		&& bps_trigger_period == other.bps_trigger_period
		&& len == other.len);
}
udp_rule& udp_rule::operator+=( udp_rule& other)
{
	if (this != &other)
	{
		count_packets += other.count_packets;
		count_bytes += other.count_bytes;
		// сбрасываем счетчик у исходного правила
		other.count_packets = 0; 
		other.count_bytes = 0;
	}
	return *this;
}
std::string udp_rule::make_info()
{
	std::string info = "udp|"
				+ ip_rule_info() + "|"
				+ (ip_src.stat() ? ip_src.to_cidr() : "") + "|"
				+ (ip_dst.stat() ? ip_dst.to_cidr() : "") + "|"
				+ (src_port.stat() ? src_port.to_range() : "") + "|"
				+ (dst_port.stat() ? dst_port.to_range() : "") + "|";
	return info;
}