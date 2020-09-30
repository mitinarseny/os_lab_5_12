#include "shared.h"

#include <sys/types.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <fcntl.h>
#include <errno.h>

#include <iomanip>
#include <cstring>
#include <system_error>
#include <array>
#include <iostream>
#include <sstream>

constexpr int ID = 1234;

key_t get_key(const std::string& path) {
	if (::creat(path.c_str(), 0666) == -1)
		throw std::system_error(errno, std::system_category(), "creat");
	key_t key = ::ftok(path.c_str(), ID);
	if (key == -1)
		throw std::system_error(errno, std::system_category(), "ftok");
	return key;
}

int get_shmem(key_t key, std::size_t size, int flags) {
	auto id = ::shmget(key, size, flags | 0666);
	if (id == -1)
		throw std::system_error(errno, std::system_category(), "shmget");
	return id;
}

int create_shmem(key_t key, std::size_t size) {
	return get_shmem(key, size, IPC_CREAT | IPC_EXCL);
}

void delete_shmem(int shm_id) {
	if (::shmctl(shm_id, IPC_RMID, nullptr) == -1)
		throw std::system_error(errno, std::system_category(), "shmctl");
}

byte_t* attach_shmem(int shm_id) {
	auto ptr = reinterpret_cast<byte_t*>(::shmat(shm_id, nullptr, 0));
	if (ptr == reinterpret_cast<byte_t*>(-1))
		throw std::system_error(errno, std::system_category(), "shmat");
	return ptr;
}

int get_sem(key_t key, int nsems, int flags) {
	auto id = ::semget(key, nsems, flags | 0666);
	if (id == -1)
		throw std::system_error(errno, std::system_category(), "semget");
	return id;
}

int create_sem(key_t key, int nsems) {
	return get_sem(key, nsems, IPC_CREAT | IPC_EXCL);
}

void delete_sem(int sem_id, int semnum) {
	if (::semctl(sem_id, semnum, IPC_RMID) == -1)
		throw std::system_error(errno, std::system_category(), "semctl");
}

void sem_add(int sem_id, unsigned short semnum, short num) {
	std::array<sembuf, 1> sops;
	sops.fill({.sem_num = semnum, .sem_op = num, .sem_flg = 0});
	while (::semop(sem_id, sops.data(), sops.size()) == -1) {
		if (errno == EINTR)
			continue;
		throw std::system_error(errno, std::system_category(), "semop");
	}
}

bool check_mode(char c, char can_be) {
	if (c == '-')
		return false;
	if (c == can_be)
		return true;
	throw std::invalid_argument(std::string("access descriptor should be '") + can_be + "', got: '" + c + "'");
}

std::istream& operator>>(std::istream& is, role_access_mode& ram) {
	char c;
	if (!(is >> c))
		return is;
	ram.r = check_mode(c, 'r');
	if (!(is >> c))
		return is;
	ram.w = check_mode(c, 'w');
	if (!(is >> c))
		return is;
	ram.x = check_mode(c, 'x');
	return is;
}

std::ostream& operator<<(std::ostream& os, const role_access_mode& ram) {
	return os << (ram.r ? 'r' : '-') << (ram.w ? 'w' : '-') << (ram.x ? 'x' : '-');
}

std::istream& operator>>(std::istream& is, access_mode& am) {
	char c;
	if (!(is >> c >> c)) // skip --
		return is;
	return is >> am.all >> am.group >> am.user;
}

std::istream& operator>>(std::istream& is, shm_segment& s) {
	std::string ignore_me;
	return is >> ignore_me // T
		>> s.ID >> ignore_me // KEY
		>> s.MODE >> s.OWNER >> s.GROUP
		>> s.CPID >> s.LPID
		>> std::get_time(&s.ATIME, "%H:%M:%S")
		>> ignore_me >> ignore_me; // DTIME, CTIME
}

std::string capture_cmd_output(const std::string& cmd) {
    FILE* stream = ::popen((cmd + " 2>&1").c_str(), "r");
	if (stream == nullptr)
		throw std::system_error(errno, std::system_category(), "popen");

	const int max_buffer = 1024;
	char buffer[max_buffer];
	std::string data;
	while (std::feof(stream) == 0)
		if (std::fgets(buffer, max_buffer, stream) != nullptr)
			data.append(buffer);
	::pclose(stream);

    return data;
}

std::ostream& operator<<(std::ostream& os, const shm_owner_access_mode& am) {
	return os << "ID: " << am.ID
			<< ", OWNER: " << am.OWNER
			<< ", OWNER ACCESS MODE: " << am.owner_access_mode;
}
