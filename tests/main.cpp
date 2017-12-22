#include "ris.hpp"

struct bulb_state {};
struct on_state : public bulb_state {
	on_state() { std::cout << "on" << std::endl; }
};
struct off_state : public bulb_state {
	off_state() { std::cout << "off" << std::endl; }
};
struct error_state : public bulb_state {
	error_state(const std::string& message) { std::cout << "error: " << message  << std::endl; }
};

int main(int argc, char *argv[]) {
	ris::transition_table<bulb_state> tt;
	tt.add<on_state, off_state>();
	tt.add<off_state, on_state>();
	tt.add<on_state, error_state>();
	tt.add<off_state, error_state>();

	ris::state_handler<bulb_state> sh(tt);
	sh.change<on_state>();
	sh.change<off_state>();
	sh.change<on_state>();
	sh.change<on_state>();
	sh.change<error_state>("bulb overheated");

#ifdef _WIN32
	std::cin.get();
#endif
	return 0;
}
