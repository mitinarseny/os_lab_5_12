#include "shared.h"

#include <iostream>
#include <vector>
#include <sstream>
#include <iomanip>

struct Resources {
	Resources(): shm_id(get_shmem(get_key(MEM_PATH), SHMEM_SIZE)),
		         sem_id(get_sem(get_key(SEM_PATH), 1)) {}

	~Resources() {
		delete_shmem(this->shm_id);
		delete_sem(this->sem_id, 0);
	}

	const int shm_id;
	const int sem_id;
};

std::vector<shm_segment> get_shm_segments() {
	const std::string ipcs_cmd = "ipcs -mpt";
	std::istringstream out(capture_cmd_output(ipcs_cmd));
	out.exceptions(std::istream::badbit);
	
	out.ignore(std::numeric_limits<std::streamsize>::max(), '\n'); // IPC status ...
	out.ignore(std::numeric_limits<std::streamsize>::max(), '\n'); // headers
	out.ignore(std::numeric_limits<std::streamsize>::max(), '\n'); // Shared Memory:

	std::vector<shm_segment> ss;
	shm_segment s;
	while (out >> s)
		ss.emplace_back(s);
		
	return ss;
}

int main() try {
	Resources res;
	std::cout << "SHM_ID: " << res.shm_id << ", SEM_ID: " << res.sem_id << std::endl;

	byte_t* ptr = attach_shmem(res.shm_id);
	std::cout << "ATTACHED SHARED MEMORY: 0x" << std::hex << uintptr_t(ptr) << std::dec << std::endl;

	auto shm_segs = get_shm_segments();
	std::cout << "got segs, size: " << shm_segs.size() << std::endl;
	ptr = write_vector(shm_segs, ptr);

	sem_add(res.sem_id, 0, 1); // release sem
	sem_add(res.sem_id, 0, -2); // wait for owner access modes
	
	auto owner_access_modes = read_vector<shm_owner_access_mode>(ptr);
	std::cout << "got " << owner_access_modes.size() << " access modes:" << std::endl;
	for (const shm_owner_access_mode& am: owner_access_modes) {
		std::cout << '\t' << am << std::endl;
	}
	
	std::cout << "Last attached processes:" << std::endl;
	for (const auto& s: shm_segs) {
		std::cout << "\tID: " << s.ID
			<< ", LPID: " << s.LPID
			<< ", ATIME: " << std::put_time(&s.ATIME, "%H:%M:%S") << std::endl;
	}

	return EXIT_SUCCESS;
} catch (const std::exception& e) {
	std::cerr << e.what() << std::endl;
	return EXIT_FAILURE;
}
