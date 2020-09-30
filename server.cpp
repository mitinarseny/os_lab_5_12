#include "shared.h"

#include <iostream>
#include <array>
#include <vector>

std::vector<shm_segment> get_shm_segments(byte_t* ptr) {
	auto seg_count = *reinterpret_cast<std::size_t*>(ptr);
	ptr += sizeof(std::size_t);

	std::vector<shm_segment> segs;
	for (std::size_t i = 0; i < seg_count; ++i, ptr += sizeof(shm_segment))
		segs.emplace_back(*reinterpret_cast<shm_segment*>(ptr));
	return segs;
}

int main() try {
	auto shm_id = create_shmem(get_key(MEM_PATH), SHMEM_SIZE);
	auto sem_id = create_sem(get_key(SEM_PATH), 1);
	std::cout << "SHM_ID: " << shm_id << ", SEM_ID: " << sem_id << std::endl;

	byte_t* ptr = attach_shmem(shm_id);
	std::cout << "ATTACHED SHARED MEMORY: 0x" << std::hex << uintptr_t(ptr) << std::dec << std::endl;

	std::cout << "waiting for shared memory segments (sem " << sem_id << ")..." << std::endl;
	sem_add(sem_id, 0, -1); // wait for client
	std::cout << "semaphore obtained!" << std::endl;

	auto v = read_vector<shm_segment>(ptr);
	ptr += sizeof(std::size_t) + sizeof(shm_segment) * v.size();
	std::cout << "got " << v.size() << " shared memory segments:" << std::endl;
	std::vector<shm_owner_access_mode> owner_access_modes;
	owner_access_modes.reserve(v.size());
	for (const shm_segment& s : v) {
		shm_owner_access_mode am{
			.ID = s.ID,
			.OWNER = s.OWNER,
			.owner_access_mode = s.MODE.user,
		};
		owner_access_modes.emplace_back(am);
		std::cout << '\t' << am << std::endl;
	}
	
	write_vector(owner_access_modes, ptr);
	sem_add(sem_id, 0, 2);

	return EXIT_SUCCESS;
} catch (const std::exception& e) {
	std::cerr << e.what() << std::endl;
	return EXIT_FAILURE;
}
