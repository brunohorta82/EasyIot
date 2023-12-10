#include <Arduino.h>
unsigned char devices_html[] PROGMEM = {
  0x1f, 0x8b, 0x08, 0x08, 0xb9, 0x4e, 0x76, 0x65, 0x00, 0x03, 0x64, 0x65,
  0x76, 0x69, 0x63, 0x65, 0x73, 0x2e, 0x6d, 0x69, 0x6e, 0x2e, 0x68, 0x74,
  0x6d, 0x6c, 0x00, 0xb5, 0x56, 0xdb, 0x6e, 0xe3, 0x36, 0x10, 0xfd, 0x15,
  0x96, 0x0f, 0xc5, 0x2e, 0xb0, 0x8a, 0xbd, 0x76, 0x8b, 0xa6, 0x69, 0xa2,
  0x45, 0x9a, 0xa4, 0xc1, 0xa2, 0x6d, 0x1a, 0xc4, 0x49, 0xd1, 0xb7, 0x82,
  0x16, 0x27, 0x12, 0x11, 0x8a, 0x54, 0x49, 0xca, 0x76, 0xfa, 0x3b, 0x7d,
  0xe8, 0x87, 0xec, 0x8f, 0x75, 0x28, 0x4a, 0xf2, 0x45, 0x92, 0xe3, 0x74,
  0x5b, 0x20, 0x70, 0xc0, 0xd1, 0x70, 0xe6, 0xcc, 0xf0, 0xcc, 0xe5, 0x94,
  0x8b, 0x05, 0x49, 0x24, 0xb3, 0xf6, 0x8c, 0x2e, 0xc5, 0x9f, 0xcc, 0x70,
  0x1a, 0x9f, 0x6e, 0xc8, 0xe6, 0x7a, 0x15, 0xcd, 0x35, 0x7f, 0xde, 0x96,
  0x3e, 0x6a, 0x93, 0x47, 0xa9, 0xd1, 0x65, 0x81, 0x72, 0xc9, 0xe6, 0x20,
  0x09, 0x8a, 0x50, 0x1e, 0xa9, 0x48, 0xb1, 0x1c, 0x50, 0x6a, 0x0b, 0xa6,
  0x1a, 0x75, 0xc9, 0x54, 0x5a, 0xcb, 0x6f, 0x74, 0x0e, 0xa7, 0x23, 0xff,
  0x31, 0x3e, 0x1d, 0x55, 0x37, 0x63, 0x72, 0x2a, 0x54, 0x51, 0xba, 0xd6,
  0x76, 0xa5, 0x49, 0x2a, 0x17, 0x89, 0x56, 0xce, 0x68, 0x49, 0x89, 0x75,
  0xcf, 0x12, 0x3c, 0x42, 0xee, 0xb2, 0x13, 0x32, 0x19, 0x8f, 0x8b, 0x15,
  0x25, 0x39, 0x5b, 0x49, 0x50, 0xa9, 0xcb, 0xce, 0xe8, 0x74, 0x4c, 0x89,
  0x7b, 0x2e, 0x50, 0xc5, 0xc1, 0xca, 0x51, 0xe2, 0x4d, 0x34, 0xa6, 0x28,
  0x11, 0x7c, 0x03, 0x1a, 0x29, 0x24, 0x4b, 0x20, 0xd3, 0x92, 0x03, 0x22,
  0x86, 0xd5, 0x09, 0x79, 0x12, 0x2e, 0xc9, 0x40, 0x51, 0x62, 0xe0, 0x8f,
  0x52, 0x18, 0xe0, 0x08, 0x0d, 0x83, 0x3d, 0x38, 0x62, 0x6e, 0xc4, 0x02,
  0x4c, 0x5f, 0xcc, 0xcd, 0x97, 0xcb, 0xea, 0x7f, 0x27, 0x6e, 0x0b, 0x12,
  0x12, 0x47, 0xb4, 0x4a, 0x32, 0xd4, 0x46, 0xc0, 0x41, 0x7f, 0x56, 0x89,
  0xdf, 0xb8, 0x4c, 0xd8, 0xb7, 0x4d, 0x28, 0x85, 0x50, 0x76, 0x20, 0x0d,
  0x9b, 0x08, 0xdb, 0x84, 0x35, 0x21, 0xb7, 0xd8, 0x74, 0xe1, 0x84, 0x56,
  0x64, 0xc1, 0x64, 0x89, 0x16, 0xbe, 0xa1, 0xf1, 0x47, 0x59, 0xe6, 0x42,
  0xb1, 0x4f, 0x7f, 0x7f, 0xfa, 0x4b, 0x93, 0xdb, 0x52, 0x5a, 0xc6, 0x35,
  0x42, 0x0c, 0x7a, 0xbb, 0xfa, 0xc7, 0x3b, 0xfa, 0x37, 0xe8, 0x8c, 0xc9,
  0x21, 0xed, 0xaf, 0x68, 0x7c, 0x65, 0x9d, 0x36, 0xd0, 0xda, 0x25, 0x97,
  0x65, 0x21, 0xf5, 0x90, 0xfe, 0xd7, 0xad, 0x7e, 0xb0, 0xbb, 0x5f, 0xfb,
  0x3d, 0x62, 0x51, 0x0e, 0x8c, 0x29, 0x0b, 0xbc, 0xf3, 0x22, 0xf4, 0xc9,
  0xb6, 0xfa, 0x7e, 0xe4, 0xd3, 0x0e, 0xf2, 0x21, 0xcd, 0x6f, 0x69, 0x7c,
  0xcd, 0x0c, 0x4b, 0x21, 0x5f, 0x6b, 0x8c, 0xc2, 0x83, 0x1e, 0x46, 0x1f,
  0xff, 0x42, 0xf8, 0xaa, 0x51, 0x59, 0x44, 0xf8, 0x5e, 0x2d, 0x99, 0xbc,
  0xe8, 0x7d, 0x1f, 0x97, 0x82, 0x2e, 0x8d, 0x6f, 0x85, 0xd2, 0x64, 0x56,
  0xce, 0xc5, 0x20, 0x9f, 0x86, 0x19, 0x73, 0xfc, 0x32, 0x61, 0x1a, 0xff,
  0x3d, 0xb1, 0x6c, 0xaa, 0x4c, 0xa2, 0x94, 0x76, 0x83, 0x23, 0x99, 0xe0,
  0xd0, 0x89, 0x90, 0xeb, 0xa5, 0xea, 0xc4, 0x38, 0x19, 0x8a, 0xd1, 0x6b,
  0xd7, 0x51, 0x5e, 0x82, 0x4d, 0x86, 0xcb, 0xe6, 0xb3, 0xc3, 0x9c, 0x74,
  0xc3, 0x9c, 0x97, 0xce, 0xe1, 0x1b, 0x87, 0x56, 0x12, 0x0e, 0xd4, 0xd7,
  0xa7, 0x14, 0xc9, 0xd3, 0x19, 0x4d, 0x0c, 0x30, 0x07, 0x6f, 0xde, 0xb6,
  0xb6, 0xe7, 0xce, 0xbf, 0x09, 0x47, 0x61, 0xb0, 0xec, 0xcf, 0x41, 0xa9,
  0x2f, 0x3a, 0xcb, 0x16, 0x28, 0xbf, 0xb8, 0xfb, 0x78, 0x7e, 0xd7, 0xc6,
  0x14, 0x7c, 0x34, 0xee, 0x3b, 0xbc, 0x19, 0x68, 0xbf, 0xe8, 0xa1, 0x34,
  0x60, 0x83, 0x53, 0x0e, 0x0b, 0x91, 0x80, 0xfd, 0x1d, 0x63, 0x7c, 0x14,
  0x29, 0x6a, 0x3a, 0xc8, 0xb1, 0xc7, 0x39, 0xa8, 0xbe, 0x9e, 0x5f, 0xdc,
  0x3f, 0x9c, 0xdf, 0xff, 0x72, 0x17, 0x2c, 0x20, 0xaa, 0x45, 0x4a, 0xaa,
  0x34, 0xf9, 0xa6, 0xe9, 0xd3, 0x94, 0x81, 0x48, 0x33, 0xd7, 0x9c, 0x56,
  0xb9, 0x54, 0xe8, 0x21, 0x73, 0xae, 0x38, 0x19, 0x8d, 0x96, 0xcb, 0xe5,
  0xd1, 0x72, 0x7a, 0xa4, 0x4d, 0x3a, 0xc2, 0x66, 0x33, 0x1e, 0xe1, 0x65,
  0x4a, 0x16, 0x02, 0x96, 0xdf, 0xeb, 0xd5, 0x19, 0x1d, 0x93, 0x31, 0x99,
  0xfa, 0xbf, 0x2a, 0x8f, 0x8b, 0xb4, 0x0f, 0x23, 0xa2, 0x5f, 0x51, 0x7c,
  0xb3, 0x0f, 0x68, 0x98, 0x60, 0x27, 0xb2, 0x58, 0x28, 0x58, 0xc2, 0x47,
  0xd8, 0xaf, 0x41, 0x25, 0x9a, 0x0b, 0x95, 0x9e, 0xd1, 0xd2, 0x3d, 0x46,
  0xc7, 0xf4, 0x03, 0xaa, 0x7d, 0x11, 0x45, 0xe4, 0x01, 0x6b, 0x9f, 0x71,
  0xe0, 0xc4, 0xe9, 0x13, 0x32, 0xfb, 0xf5, 0x9a, 0xdc, 0x41, 0xa1, 0xdf,
  0x11, 0x8f, 0x05, 0x9d, 0x18, 0x3c, 0x1c, 0x25, 0x3a, 0x7f, 0x47, 0xae,
  0x41, 0x81, 0x61, 0x58, 0xac, 0x6b, 0x2d, 0xf2, 0xb3, 0x58, 0x81, 0x21,
  0xf7, 0x5a, 0x4b, 0x4b, 0xa2, 0x08, 0x0d, 0x06, 0xce, 0x6c, 0xbd, 0x45,
  0x83, 0xac, 0x1e, 0x54, 0xbb, 0xec, 0x0a, 0xdc, 0xad, 0x75, 0xed, 0xd2,
  0xcf, 0x06, 0xd4, 0x0a, 0x23, 0x6a, 0xdd, 0xa9, 0x9d, 0x4e, 0x53, 0x09,
  0xb3, 0xea, 0x73, 0xd3, 0xa9, 0x03, 0x6b, 0x70, 0x94, 0x24, 0x4f, 0x75,
  0xd4, 0x9b, 0x6e, 0xad, 0xc4, 0xda, 0x30, 0x04, 0xcb, 0x44, 0xf1, 0x1e,
  0xb7, 0xbb, 0x0e, 0x6c, 0x86, 0xcc, 0x00, 0x73, 0x0b, 0x26, 0x01, 0xe5,
  0xb0, 0xcf, 0x34, 0x5e, 0x1a, 0x73, 0xe1, 0x7b, 0x14, 0xcc, 0xe2, 0x30,
  0x14, 0x98, 0xd7, 0x71, 0x35, 0x14, 0x31, 0xbf, 0xe3, 0x76, 0x1e, 0x1a,
  0x6f, 0x8e, 0x6e, 0x13, 0x6c, 0xd4, 0x30, 0x64, 0x87, 0x2b, 0xb3, 0xab,
  0x9b, 0x59, 0xc3, 0x94, 0xde, 0x77, 0xac, 0xd8, 0x33, 0xfc, 0xfe, 0xaf,
  0x4f, 0x77, 0x17, 0xd0, 0x6e, 0x21, 0x78, 0x5c, 0xb9, 0xe6, 0x4c, 0xb6,
  0x91, 0x87, 0xd3, 0x16, 0xc8, 0x4a, 0x54, 0x15, 0x39, 0xe6, 0xaa, 0xef,
  0x53, 0x06, 0x8c, 0x77, 0x46, 0x74, 0x22, 0xb5, 0x45, 0x48, 0x5f, 0x3a,
  0x91, 0x83, 0xfd, 0xae, 0x41, 0x96, 0x4d, 0xb7, 0x37, 0x11, 0x8f, 0x39,
  0x9b, 0xf6, 0xd4, 0x66, 0xb0, 0xec, 0xdb, 0x4b, 0x67, 0x25, 0xf8, 0x3f,
  0x56, 0xa0, 0xd7, 0x6e, 0x3b, 0x9f, 0xb1, 0xeb, 0x44, 0x2c, 0x21, 0xdd,
  0xe8, 0xe2, 0x7b, 0x81, 0x15, 0xc6, 0x81, 0x6c, 0x8c, 0xd3, 0xf6, 0x1d,
  0x7b, 0xf3, 0x12, 0x19, 0xbd, 0x6c, 0xfa, 0x4e, 0x08, 0xb0, 0xe1, 0x24,
  0x17, 0xba, 0x01, 0x8a, 0x8d, 0x18, 0x6f, 0x40, 0x54, 0x94, 0x36, 0x5b,
  0x87, 0x52, 0x4b, 0x69, 0x33, 0x6e, 0xc7, 0xb4, 0x2d, 0xcd, 0xee, 0xb5,
  0x48, 0xce, 0xd7, 0xe3, 0x65, 0xcb, 0x5e, 0xbc, 0x1e, 0xe3, 0x5b, 0x84,
  0x3b, 0x0c, 0x12, 0x32, 0x32, 0xd9, 0x83, 0xe9, 0xfd, 0x10, 0xa6, 0xea,
  0x5e, 0x2f, 0xa8, 0x60, 0x31, 0x6e, 0x96, 0x90, 0x5d, 0x4c, 0xb5, 0x9d,
  0x2a, 0x26, 0x47, 0x0f, 0x4c, 0x5a, 0x14, 0x7a, 0xd1, 0x30, 0xce, 0xc9,
  0xde, 0xdc, 0x85, 0xdb, 0x83, 0x29, 0x6c, 0xac, 0x0f, 0x65, 0xb2, 0xaf,
  0x5e, 0x1f, 0xa3, 0x84, 0x49, 0x31, 0xc7, 0xe6, 0x2c, 0xfc, 0xfc, 0xdc,
  0xa4, 0x95, 0x5f, 0x10, 0xfa, 0xb8, 0x75, 0x11, 0x2e, 0x54, 0xdb, 0xe5,
  0xbf, 0x23, 0x95, 0x2a, 0xf3, 0xb9, 0x6f, 0x84, 0x01, 0x00, 0x6e, 0x4a,
  0x3d, 0xd4, 0x69, 0x8b, 0x11, 0x0f, 0x51, 0x26, 0x5c, 0x1b, 0xb1, 0x5f,
  0xac, 0x2c, 0xa4, 0xd8, 0x9a, 0xb5, 0x25, 0x05, 0xee, 0x75, 0xc4, 0x86,
  0x0d, 0x6b, 0x3f, 0x6b, 0xb6, 0x7d, 0x56, 0x9b, 0x4b, 0x0f, 0x39, 0x06,
  0xbd, 0x86, 0x55, 0x67, 0xdb, 0x2f, 0xaf, 0x77, 0x9e, 0x03, 0x92, 0xfc,
  0xa4, 0x56, 0x74, 0xb8, 0x19, 0xc5, 0x3f, 0xde, 0xfc, 0xf6, 0x5f, 0x64,
  0x92, 0xe1, 0x3e, 0xb3, 0x99, 0xcb, 0xc3, 0x72, 0x21, 0x85, 0x82, 0xd7,
  0xdf, 0xca, 0x21, 0x9c, 0x3a, 0xf7, 0x06, 0xd6, 0xa3, 0x26, 0x16, 0xed,
  0xaa, 0xe6, 0xbe, 0x7f, 0x7b, 0xe3, 0xb8, 0xe9, 0x39, 0xf8, 0x21, 0x8c,
  0xa3, 0x9d, 0x69, 0xea, 0x17, 0xb7, 0xf0, 0x7d, 0xbd, 0xc8, 0xd5, 0xe7,
  0xf8, 0xf2, 0xea, 0xa7, 0xab, 0xfb, 0xab, 0x76, 0x55, 0x23, 0x2f, 0xb8,
  0x61, 0x45, 0x21, 0x9f, 0x6b, 0x2f, 0x17, 0xd5, 0x30, 0xb7, 0x3d, 0xce,
  0x76, 0xb7, 0xc6, 0xfa, 0x3c, 0xb8, 0x35, 0x3e, 0xdc, 0x5e, 0x9e, 0x7b,
  0x10, 0x7b, 0xd6, 0xc6, 0xea, 0xf7, 0x1f, 0xa7, 0x93, 0xe9, 0xb0, 0xd0,
  0x0f, 0x00, 0x00
};
unsigned int devices_html_len = 1191;
