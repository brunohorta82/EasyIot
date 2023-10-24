#include <Arduino.h>
unsigned char node_html[] PROGMEM = {
  0x1f, 0x8b, 0x08, 0x08, 0xf9, 0xac, 0x37, 0x65, 0x00, 0x03, 0x6e, 0x6f,
  0x64, 0x65, 0x2e, 0x6d, 0x69, 0x6e, 0x2e, 0x68, 0x74, 0x6d, 0x6c, 0x00,
  0xad, 0x57, 0xdd, 0x72, 0xda, 0x38, 0x14, 0x7e, 0x15, 0x8d, 0xae, 0xda,
  0x0b, 0x13, 0x02, 0x4d, 0x67, 0x77, 0x07, 0x98, 0x69, 0xc3, 0xa4, 0xcb,
  0x45, 0xba, 0x6c, 0x49, 0xa6, 0xd7, 0xb2, 0x7c, 0xc0, 0x1a, 0x64, 0xc9,
  0x95, 0xe4, 0x90, 0xec, 0x6b, 0xec, 0x13, 0x74, 0xf6, 0xa2, 0x0f, 0xc2,
  0x8b, 0xed, 0x91, 0x8d, 0x89, 0xc1, 0x26, 0xe0, 0x84, 0x0b, 0x7b, 0x6c,
  0xc9, 0x3a, 0xfa, 0x7e, 0x8e, 0x8e, 0xac, 0x41, 0x24, 0x1e, 0x08, 0x97,
  0xcc, 0xda, 0x21, 0x0d, 0xf5, 0x23, 0xc1, 0x2b, 0x48, 0x8d, 0x48, 0x98,
  0x79, 0xa2, 0xa3, 0xc1, 0x6e, 0x67, 0x10, 0x03, 0x8b, 0xc0, 0x60, 0x7b,
  0xdc, 0xaf, 0x36, 0x3b, 0xe1, 0x24, 0x60, 0xab, 0x4d, 0x99, 0x2a, 0xdb,
  0x25, 0x53, 0x8b, 0x40, 0xe9, 0x08, 0xdb, 0xbf, 0xae, 0xff, 0x1d, 0x5c,
  0xf8, 0xbe, 0xd1, 0xe0, 0x22, 0xee, 0xe3, 0x0d, 0xa3, 0x8e, 0x06, 0x73,
  0x6d, 0x12, 0xbc, 0x0b, 0x90, 0x91, 0x05, 0x47, 0x44, 0x34, 0xa4, 0xf3,
  0x39, 0x25, 0x91, 0xb0, 0x2c, 0x94, 0x80, 0x6f, 0xe5, 0x53, 0x1d, 0x46,
  0xa8, 0xa3, 0x3d, 0x70, 0x3e, 0x58, 0xb0, 0x30, 0x3a, 0x4b, 0xb1, 0x5d,
  0xb2, 0x10, 0x24, 0xc1, 0xa6, 0x21, 0xf5, 0x00, 0x26, 0x51, 0x23, 0x34,
  0x96, 0x78, 0x68, 0x3a, 0x81, 0x2d, 0xb6, 0x7c, 0xdc, 0x88, 0x0c, 0x84,
  0x4a, 0x33, 0xb7, 0x13, 0x99, 0x6b, 0xe5, 0x8c, 0x96, 0x94, 0x24, 0xec,
  0x51, 0x82, 0x5a, 0xb8, 0x78, 0x48, 0xfb, 0x5d, 0x4a, 0xdc, 0x53, 0x0a,
  0x43, 0xea, 0xe0, 0xd1, 0xd1, 0x9c, 0xc0, 0x66, 0x3a, 0x92, 0x4a, 0xc6,
  0x21, 0xd6, 0x12, 0xc5, 0x1a, 0x52, 0x78, 0xfc, 0x83, 0x2c, 0x85, 0xe3,
  0x31, 0x28, 0x4a, 0x0c, 0xfc, 0xc8, 0x84, 0x81, 0xa8, 0x54, 0xa1, 0xb8,
  0xbf, 0x51, 0x66, 0x0b, 0x3c, 0x33, 0xc2, 0xa1, 0x26, 0x33, 0x58, 0x64,
  0x86, 0xa9, 0xf5, 0x2f, 0xd6, 0xa0, 0xf8, 0xab, 0x55, 0x64, 0x9c, 0x83,
  0xb5, 0x53, 0x2d, 0x94, 0x9b, 0xe2, 0x87, 0x2b, 0x6d, 0x1a, 0x25, 0x4d,
  0xb7, 0x7d, 0x53, 0x26, 0xd9, 0x83, 0x61, 0xc4, 0x7f, 0x5d, 0xea, 0x4b,
  0x3e, 0x4d, 0xf7, 0x25, 0x66, 0x99, 0xd3, 0x5c, 0x27, 0xa9, 0x04, 0x87,
  0x32, 0x6a, 0x6f, 0x7f, 0xa3, 0xea, 0x85, 0xcc, 0xdb, 0xf0, 0xb9, 0xd4,
  0x4d, 0x98, 0xea, 0xba, 0xdb, 0x2c, 0x05, 0x53, 0xc1, 0x5c, 0x13, 0xe2,
  0x20, 0xe5, 0x54, 0xdc, 0xdb, 0xdc, 0x83, 0x1a, 0xcd, 0x0c, 0xdb, 0x8b,
  0xec, 0xb9, 0x77, 0x42, 0x8a, 0x7f, 0x58, 0xa4, 0xcd, 0x81, 0x1c, 0x6a,
  0x47, 0xf0, 0x39, 0x8f, 0xca, 0xd9, 0xeb, 0x84, 0x58, 0x94, 0x08, 0xd5,
  0x92, 0xc8, 0xeb, 0x3d, 0x3b, 0xab, 0x61, 0x15, 0x20, 0xa7, 0x1a, 0xf5,
  0x8a, 0xd5, 0xf1, 0x7d, 0x12, 0xdc, 0x4c, 0xce, 0x92, 0xf5, 0x2b, 0x31,
  0x17, 0xb3, 0xd9, 0x64, 0x8c, 0xab, 0x0a, 0xef, 0x6d, 0xcb, 0x43, 0xaf,
  0x4f, 0x9b, 0xe4, 0xda, 0x77, 0x7a, 0x3b, 0x49, 0x5d, 0x92, 0x04, 0x32,
  0xdf, 0xdb, 0xc6, 0xec, 0x3c, 0x1a, 0x70, 0x03, 0xee, 0xed, 0x5e, 0x1f,
  0x23, 0xf8, 0xb1, 0xfb, 0x02, 0xc1, 0x5d, 0xeb, 0x2b, 0xb0, 0x0e, 0x38,
  0x9f, 0x9e, 0xe4, 0xfc, 0xc6, 0xbb, 0x0a, 0xe1, 0x28, 0xe6, 0x28, 0xc1,
  0xf8, 0xcf, 0xeb, 0x4a, 0x6d, 0x29, 0xba, 0x37, 0xc3, 0xec, 0xca, 0x57,
  0x5e, 0x1c, 0x54, 0x90, 0x7a, 0x60, 0x32, 0xf3, 0xfa, 0x9b, 0x0c, 0x28,
  0xf1, 0x8b, 0x78, 0x13, 0x22, 0x87, 0x59, 0x3c, 0x69, 0xc5, 0x63, 0x54,
  0xcb, 0x7f, 0xa5, 0x17, 0x0b, 0x09, 0xb3, 0x3c, 0xc2, 0x3b, 0x17, 0x0b,
  0xfb, 0xbe, 0xa4, 0x87, 0xb5, 0x9c, 0x2f, 0x11, 0x11, 0xc5, 0xe9, 0xaa,
  0x32, 0x5b, 0x29, 0x90, 0x17, 0x41, 0x63, 0x54, 0x4e, 0x65, 0x47, 0xd8,
  0x73, 0xe4, 0xe3, 0x04, 0xdb, 0x26, 0xb5, 0x3a, 0x7a, 0x52, 0x4d, 0xd9,
  0x0c, 0xaf, 0x5a, 0x78, 0x79, 0xd5, 0xe0, 0xc7, 0xe5, 0xef, 0xbd, 0xce,
  0xe5, 0xc7, 0xdf, 0x3a, 0x97, 0x9d, 0x5e, 0xb7, 0x6d, 0xee, 0x7d, 0x59,
  0xd1, 0xd1, 0x17, 0xe6, 0x60, 0xc5, 0x9e, 0x4e, 0xc2, 0x58, 0xe2, 0xc2,
  0x71, 0x6d, 0x70, 0x5d, 0x7d, 0x68, 0x0b, 0xec, 0x96, 0xd9, 0x65, 0xe3,
  0x5f, 0x00, 0xb8, 0x24, 0xef, 0xba, 0x5d, 0xff, 0xb4, 0x9c, 0xe1, 0x92,
  0x88, 0x80, 0x7c, 0x83, 0xe8, 0xb5, 0x8b, 0xc2, 0x23, 0x2f, 0x49, 0xe5,
  0x73, 0xd6, 0x79, 0xf4, 0xae, 0xae, 0x3a, 0xe5, 0xd5, 0x3d, 0x29, 0xdf,
  0xab, 0xa8, 0x79, 0x2c, 0x36, 0xd9, 0x2a, 0x43, 0x19, 0x24, 0x8c, 0xd3,
  0x51, 0x67, 0x8b, 0xb5, 0x7d, 0xb5, 0xbc, 0xfd, 0xfb, 0xee, 0xee, 0x2c,
  0xc5, 0x32, 0xf9, 0xe1, 0xdc, 0x24, 0x1d, 0x2b, 0x4b, 0x47, 0x9f, 0x8d,
  0x5e, 0xe2, 0x22, 0x98, 0x4c, 0xc9, 0x05, 0x19, 0x7f, 0x9d, 0xb5, 0x4e,
  0xd6, 0x8a, 0x9c, 0x1f, 0xba, 0x05, 0xd7, 0xe7, 0xe8, 0x2f, 0xe5, 0x45,
  0xbf, 0x55, 0xbe, 0xfa, 0x98, 0xf7, 0xdb, 0xad, 0xfc, 0x4d, 0xdb, 0x7c,
  0x23, 0xa5, 0x23, 0xc5, 0x7f, 0xef, 0x4f, 0xb2, 0x64, 0xb9, 0x45, 0x54,
  0x27, 0x2a, 0x58, 0xd2, 0x96, 0xdf, 0xf9, 0x76, 0xfe, 0xa3, 0xbb, 0x41,
  0xaf, 0x5e, 0xfa, 0x1b, 0x14, 0x28, 0x69, 0x1e, 0xfb, 0x13, 0xb0, 0x38,
  0x81, 0x5a, 0x1c, 0xd8, 0x15, 0x2e, 0xca, 0x43, 0x83, 0x7f, 0xcc, 0x4f,
  0x11, 0x6f, 0x3d, 0xa6, 0xc0, 0x2a, 0x98, 0x0b, 0xdf, 0xf7, 0xc9, 0x65,
  0xcc, 0x7b, 0xbd, 0xfe, 0xb5, 0xfe, 0x4f, 0xfb, 0x5a, 0x70, 0x23, 0x4c,
  0xb2, 0x62, 0x06, 0x0e, 0x1c, 0x60, 0xb0, 0xd4, 0x4b, 0x28, 0x64, 0x41,
  0x39, 0xc0, 0xc5, 0x1a, 0x09, 0x4e, 0xff, 0x9a, 0xdd, 0x21, 0x7b, 0xee,
  0x84, 0x56, 0x43, 0x7a, 0x91, 0xa5, 0x11, 0x96, 0x44, 0x4a, 0x40, 0xf1,
  0x42, 0xa1, 0x24, 0x93, 0x4e, 0xa4, 0xcc, 0xb8, 0x1c, 0x7d, 0x80, 0xbd,
  0xec, 0xe0, 0x19, 0xa7, 0x50, 0x3f, 0x3f, 0x1c, 0x6d, 0x90, 0x4c, 0x7c,
  0xcb, 0x8d, 0x47, 0xbb, 0xd9, 0xc0, 0xca, 0xf8, 0x45, 0xf0, 0x82, 0xc7,
  0x8b, 0xcb, 0x39, 0xcc, 0x9c, 0xd3, 0x6a, 0xf3, 0xbd, 0xcd, 0xc2, 0x44,
  0xb8, 0xed, 0x7f, 0x5c, 0xe8, 0x14, 0xc1, 0xab, 0x72, 0x04, 0xac, 0xa9,
  0x25, 0x94, 0x75, 0x4c, 0xca, 0x5c, 0xb1, 0xe0, 0x01, 0xbd, 0x42, 0x9a,
  0xb8, 0x2b, 0xe5, 0xad, 0xcc, 0x90, 0xb9, 0xc0, 0xdd, 0x51, 0x18, 0xbd,
  0x55, 0xac, 0x98, 0xee, 0xd9, 0xbc, 0xe2, 0xdc, 0xb7, 0xef, 0x9b, 0x7d,
  0xb2, 0x0e, 0x92, 0x60, 0xae, 0xb5, 0xcb, 0xad, 0xdb, 0x01, 0x59, 0xbc,
  0xe4, 0x7b, 0xb3, 0x14, 0x7c, 0x89, 0x40, 0x34, 0x8b, 0xc6, 0x30, 0x67,
  0xa8, 0xa4, 0x7d, 0xf7, 0xbe, 0x86, 0x3e, 0xf2, 0x1b, 0x78, 0xe3, 0xcf,
  0xbb, 0x01, 0xcc, 0x9b, 0x60, 0x8e, 0xe6, 0x68, 0xcf, 0xee, 0x9a, 0x19,
  0x03, 0x0b, 0x44, 0x7d, 0xad, 0xd5, 0x5c, 0xf8, 0xb3, 0xd3, 0xb3, 0xf3,
  0xeb, 0x9f, 0xa1, 0x11, 0x9c, 0xed, 0xf3, 0x20, 0x47, 0xa0, 0x19, 0x08,
  0x91, 0x43, 0x03, 0x28, 0x34, 0x4f, 0x09, 0xb5, 0x38, 0x80, 0xca, 0x61,
  0x3e, 0xd0, 0xd1, 0x37, 0x10, 0x4a, 0x70, 0xc1, 0xcc, 0xcb, 0xea, 0xe5,
  0x2f, 0xff, 0x03, 0x0b, 0xb0, 0x79, 0x35, 0xba, 0x0f, 0x00, 0x00
};
unsigned int node_html_len = 1031;
