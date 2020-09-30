#ifndef SHARED_H
#define SHARED_H

#include <sys/ipc.h>

#include <cstddef>
#include <string>
#include <istream>

using byte_t = unsigned char;

const std::string MEM_PATH = "/tmp/os_lab_shmem_m";
const std::string SEM_PATH = "/tmp/os_lab_shmem_s";

constexpr std::size_t SHMEM_SIZE = 1 << 16;

template<typename T>
byte_t* write_vector(const std::vector<T>& what, byte_t* where) {
	*reinterpret_cast<std::size_t*>(where) = what.size();
	where += sizeof(std::size_t);

	for (std::size_t i = 0; i < what.size(); ++i, where += sizeof(T))
		*reinterpret_cast<T*>(where) = what[i];
	return where;
}

template<typename T>
std::vector<T> read_vector(byte_t* from) {
	std::size_t size = *reinterpret_cast<std::size_t*>(from);
	from += sizeof(std::size_t);

	std::vector<T> v;
	v.reserve(size);
	for (std::size_t i = 0; i < size; ++i, from += sizeof(T))
		v.emplace_back(*reinterpret_cast<T*>(from));
	return v;
}

struct role_access_mode {
	bool r, w, x;
	role_access_mode(): r(), w(), x() {}
};

std::istream& operator>>(std::istream& is, role_access_mode& ram);
std::ostream& operator<<(std::ostream& os, const role_access_mode& ram);

struct access_mode {
	role_access_mode all, group, user;
	access_mode(): all(), group(), user() {}
};

std::istream& operator>>(std::istream& is, access_mode& am);

struct shm_segment {
	int ID;
	// key_t KEY;
	access_mode MODE;
	std::string OWNER, GROUP;
	int CPID, LPID;
	std::tm ATIME;
};

struct shm_owner_access_mode {
	int ID;
	std::string OWNER;
	role_access_mode owner_access_mode;
};

std::ostream& operator<<(std::ostream& os, const shm_owner_access_mode& am);

std::istream& operator>>(std::istream& is, shm_segment& s);

key_t get_key(const std::string& path);

int get_shmem(key_t key, std::size_t size, int flags = 0);
int create_shmem(key_t key, std::size_t size);
byte_t* attach_shmem(int shm_id);
void delete_shmem(int shm_id);

int get_sem(key_t key, int nsems, int flags = 0);
int create_sem(key_t key, int nsems);
void sem_add(int sem_id, unsigned short semnum, short num);
void delete_sem(int sem_id, int semnum);

std::string capture_cmd_output(const std::string& cmd);

#endif // SHARED_H
