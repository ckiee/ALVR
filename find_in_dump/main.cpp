#include <cmath>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

struct HmdMatrix34_t {
    float m[3][4];
};

struct HmdVector3_t {
    float v[3];
};

struct TrackedDevicePose_t {
    HmdMatrix34_t mDeviceToAbsoluteTracking;
    HmdVector3_t vVelocity;        // velocity in tracker space in m/s
    HmdVector3_t vAngularVelocity; // angular velocity in radians/s (?)
    int eTrackingResult;
    char bPoseIsValid;

    // This indicates that there is a device connected for this spot in the pose array.
    // It could go from true to false if the user unplugs the device.
    char bDeviceIsConnected;
};

const TrackedDevicePose_t &find_pose_in_call_stack();
inline HmdMatrix34_t transposeMul33(const HmdMatrix34_t &a) {
    HmdMatrix34_t result;
    for (unsigned i = 0; i < 3; i++) {
        for (unsigned k = 0; k < 3; k++) {
            result.m[i][k] = a.m[k][i];
        }
    }
    result.m[0][3] = a.m[0][3];
    result.m[1][3] = a.m[1][3];
    result.m[2][3] = a.m[2][3];
    return result;
}

inline HmdMatrix34_t matMul33(const HmdMatrix34_t &a, const HmdMatrix34_t &b) {
    HmdMatrix34_t result;
    for (unsigned i = 0; i < 3; i++) {
        for (unsigned j = 0; j < 3; j++) {
            result.m[i][j] = 0.0f;
            for (unsigned k = 0; k < 3; k++) {
                result.m[i][j] += a.m[i][k] * b.m[k][j];
            }
        }
    }
    return result;
}
void hexDump (
    const char * desc,
    const void * addr,
    const int len,
    int perLine
) {
    // Silently ignore silly per-line values.

    if (perLine < 4 || perLine > 64) perLine = 16;

    int i;
    unsigned char buff[perLine+1];
    const unsigned char * pc = (const unsigned char *)addr;

    // Output description if given.

    if (desc != NULL) printf ("%s:\n", desc);

    // Length checks.

    if (len == 0) {
        printf("  ZERO LENGTH\n");
        return;
    }
    if (len < 0) {
        printf("  NEGATIVE LENGTH: %d\n", len);
        return;
    }

    // Process every byte in the data.

    for (i = 0; i < len; i++) {
        // Multiple of perLine means new or first line (with line offset).

        if ((i % perLine) == 0) {
            // Only print previous-line ASCII buffer for lines beyond first.

            if (i != 0) printf ("  %s\n", buff);

            // Output the offset of current line.

            printf ("  %04x ", i);
        }

        // Now the hex code for the specific character.

        printf (" %02x", pc[i]);

        // And buffer a printable ASCII character for later.

        if ((pc[i] < 0x20) || (pc[i] > 0x7e)) // isprint() may be better.
            buff[i % perLine] = '.';
        else
            buff[i % perLine] = pc[i];
        buff[(i % perLine) + 1] = '\0';
    }

    // Pad out last line if not exactly perLine characters.

    while ((i % perLine) != 0) {
        printf ("   ");
        i++;
    }

    // And print the final ASCII buffer.

    printf ("  %s\n", buff);
}

bool check_pose(const TrackedDevicePose_t &p) {
    if (p.bPoseIsValid != 1 or p.bDeviceIsConnected != 1)
        return false;

    if (p.eTrackingResult != 200)
        return false;

    // auto m = matMul33(p.mDeviceToAbsoluteTracking, transposeMul33(p.mDeviceToAbsoluteTracking));
    // for (int i = 0; i < 3; ++i) {
    //     for (int j = 0; j < 3; ++j) {
    //         if (std::abs(m.m[i][j] - (i == j)) > 0.1)
    //             return false;
    //     }
    // }
    return true;
}

char dump[10000000]; // 10Mb, in reality we use ~8Mb

int main(int argc, char **argv) {
    if (argc != 2) {
        printf("usage: <pid>\n");
        exit(1);
    }
    std::ostringstream maps_path;
    maps_path << "/proc/" << argv[1] << "/maps";
    std::ifstream ifs(maps_path.str(), std::ifstream::in);
    if (ifs.fail()) {
        perror("open");
        exit(1);
    }

    int fill_ptr = 0;
    std::string line;
    while (std::getline(ifs, line)) {
        int spaces = 0;
        bool got_path = false;
        for (long unsigned int i = 0; i < line.length(); i++) {
            if (line[i] == ' ') {
                spaces++;
            } else if (!got_path && spaces > 10 && line[i] == '/') {
                const auto DEV_SHM = "/dev/shm";
                if (strncmp(&line[i], DEV_SHM, strlen(DEV_SHM)) == 0) {
                    // printf("%d:%s\n", i, &line[i]);
                    std::string shm_path = &line[i];
                    std::ifstream shm(shm_path, std::ifstream::in);

                    char c = shm.get();

                    while (shm.good()) {
                        dump[fill_ptr] = c;
                        fill_ptr++;
                        c = shm.get();
                    }

                    shm.close();
                }
                got_path = true;
            }
        }
    }

    // std::cout << "collected " << fill_ptr << " bytes of shm\n";

    int check_ptr = 0;
    while (check_ptr < fill_ptr) {
        bool check = check_pose(*(TrackedDevicePose_t *)&dump[check_ptr]);
        if (check) {
            TrackedDevicePose_t *pose = (TrackedDevicePose_t*)&dump[check_ptr];
            // char title[256];
            // snprintf(title, 256, "TrackedDevicePose_t @ 0x%x", check_ptr);
            std::cout << "-----\n";
            std::cout << pose->mDeviceToAbsoluteTracking.m[0][0] << "," << pose->mDeviceToAbsoluteTracking.m[0][1] << "," << pose->mDeviceToAbsoluteTracking.m[0][2] << "," << pose->mDeviceToAbsoluteTracking.m[0][3] << "\n";
            std::cout << pose->mDeviceToAbsoluteTracking.m[1][0] << "," << pose->mDeviceToAbsoluteTracking.m[1][1] << "," << pose->mDeviceToAbsoluteTracking.m[1][2] << "," << pose->mDeviceToAbsoluteTracking.m[1][3] << "\n";
            std::cout << pose->mDeviceToAbsoluteTracking.m[2][0] << "," << pose->mDeviceToAbsoluteTracking.m[2][1] << "," << pose->mDeviceToAbsoluteTracking.m[2][2] << "," << pose->mDeviceToAbsoluteTracking.m[2][3] << "\n";
            // hexDump(title, &pose->mDeviceToAbsoluteTracking, sizeof(pose->mDeviceToAbsoluteTracking), 16);
            break;
        }
        check_ptr += sizeof(TrackedDevicePose_t);
    }

    ifs.close();

    return 0;
}
