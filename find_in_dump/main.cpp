#include <cmath>
#include <cstdio>
#include <iostream>
#include <fstream>

struct HmdMatrix34_t
{
  float m[3][4];
};

struct HmdVector3_t
{
  float v[3];
};

struct TrackedDevicePose_t
{
  HmdMatrix34_t mDeviceToAbsoluteTracking;
  HmdVector3_t vVelocity;       // velocity in tracker space in m/s
  HmdVector3_t vAngularVelocity;    // angular velocity in radians/s (?)
  int eTrackingResult;
  char bPoseIsValid;

  // This indicates that there is a device connected for this spot in the pose array.
  // It could go from true to false if the user unplugs the device.
  char bDeviceIsConnected;
};

const TrackedDevicePose_t & find_pose_in_call_stack();
inline HmdMatrix34_t transposeMul33(const HmdMatrix34_t& a) {
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


inline HmdMatrix34_t matMul33(const HmdMatrix34_t& a, const HmdMatrix34_t& b) {
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


bool check_pose(const TrackedDevicePose_t & p)
{
  if (p.bPoseIsValid != 1 or p.bDeviceIsConnected != 1)
    return false;

  if (p.eTrackingResult != 200)
    return false;

  auto m = matMul33(p.mDeviceToAbsoluteTracking, transposeMul33(p.mDeviceToAbsoluteTracking));
  for (int i = 0 ; i < 3; ++i )
  {
    for (int j = 0 ; j < 3 ; ++j)
    {
      if (std::abs(m.m[i][j] - (i == j)) > 0.1)
        return false;
    }
  }
  return true;
}

char dump[9000000];

int main(int argc, char** argv) {
  std::ifstream ifs("/home/ckie/dump", std::ifstream::in);

  char c = ifs.get();

  int fill_ptr = 0;
  while (ifs.good()) {
    dump[fill_ptr] = c;
    fill_ptr++;
    c = ifs.get();
  }

  int check_ptr = 0;
  while (check_ptr < fill_ptr) {
    bool check = check_pose(*(TrackedDevicePose_t*)&dump[check_ptr]);
    if (check) {
      printf("found TrackedDevicePose_t at %x\n", check_ptr);
    }
    check_ptr+=sizeof(TrackedDevicePose_t);
  }

  ifs.close();

  return 0;
}
