#include <Arduino.h>
unsigned char node_html[] PROGMEM = {
  0x1f, 0x8b, 0x08, 0x08, 0xbf, 0x81, 0xe6, 0x64, 0x00, 0x03, 0x6e, 0x6f,
  0x64, 0x65, 0x2e, 0x6d, 0x69, 0x6e, 0x2e, 0x68, 0x74, 0x6d, 0x6c, 0x00,
  0xad, 0x57, 0xc1, 0x72, 0xe2, 0x38, 0x10, 0xfd, 0x15, 0x95, 0x4e, 0x33,
  0x07, 0x08, 0x81, 0xc9, 0xd4, 0xee, 0x16, 0x50, 0x35, 0x13, 0x2a, 0xb3,
  0x1c, 0x32, 0xcb, 0x0e, 0x49, 0xcd, 0x59, 0x96, 0xdb, 0x58, 0x85, 0x2c,
  0x79, 0x24, 0x39, 0x24, 0xfb, 0x1b, 0xfb, 0x05, 0x53, 0x7b, 0x98, 0x0f,
  0xe1, 0xc7, 0xb6, 0x65, 0x63, 0x62, 0xb0, 0x09, 0x38, 0xe1, 0x60, 0x97,
  0xdd, 0xb2, 0x5b, 0xef, 0xf5, 0x6b, 0x75, 0x4b, 0xc3, 0x50, 0x3c, 0x10,
  0x2e, 0x99, 0xb5, 0x23, 0x1a, 0xe8, 0x47, 0x82, 0x57, 0x27, 0x35, 0x22,
  0x61, 0xe6, 0x89, 0x8e, 0x87, 0xbb, 0x83, 0x9d, 0x18, 0x58, 0x08, 0x06,
  0xed, 0xf1, 0xa0, 0x6a, 0x76, 0xc2, 0x49, 0x40, 0xab, 0x4d, 0x99, 0x2a,
  0xed, 0x92, 0xa9, 0x45, 0x47, 0xe9, 0x10, 0xed, 0x5f, 0xd7, 0xff, 0x0e,
  0x2f, 0xfc, 0xd8, 0x78, 0x78, 0x11, 0x0f, 0xf0, 0x86, 0x5e, 0xc7, 0xc3,
  0x48, 0x9b, 0x04, 0xef, 0x02, 0x64, 0x68, 0xc1, 0x11, 0x11, 0x8e, 0x68,
  0x14, 0x51, 0x12, 0x0a, 0xcb, 0x02, 0x09, 0xf8, 0x56, 0x3e, 0xd5, 0x61,
  0x04, 0x3a, 0xdc, 0x03, 0xe7, 0x9d, 0x75, 0x16, 0x46, 0x67, 0x29, 0xda,
  0x25, 0x0b, 0x40, 0x12, 0x34, 0x8d, 0xa8, 0x07, 0x30, 0x0d, 0x1b, 0xa1,
  0xb1, 0xc4, 0x43, 0xd3, 0x09, 0x6c, 0xb1, 0xe5, 0xff, 0x8d, 0xc9, 0x50,
  0xa8, 0x34, 0x73, 0x3b, 0x9e, 0xb9, 0x56, 0xce, 0x68, 0x49, 0x49, 0xc2,
  0x1e, 0x25, 0xa8, 0x85, 0x8b, 0x47, 0x74, 0xd0, 0xa3, 0xc4, 0x3d, 0xa5,
  0x30, 0xa2, 0x0e, 0x1e, 0x1d, 0xcd, 0x09, 0x6c, 0xa6, 0x23, 0xa9, 0x64,
  0x1c, 0x62, 0x2d, 0x31, 0x58, 0x23, 0x0a, 0x8f, 0x7f, 0x90, 0xa5, 0x70,
  0x3c, 0x06, 0x45, 0x89, 0x81, 0x1f, 0x99, 0x30, 0x10, 0x96, 0x51, 0x28,
  0xee, 0xcd, 0xfc, 0x72, 0xcc, 0xde, 0xad, 0x0c, 0x64, 0x87, 0xc7, 0x22,
  0xa5, 0xe5, 0x47, 0xf9, 0xcb, 0xb8, 0xdb, 0xed, 0x6e, 0xb1, 0x37, 0xb9,
  0x69, 0xab, 0x96, 0x05, 0x9e, 0x19, 0xe1, 0x70, 0xea, 0x39, 0x2c, 0x32,
  0xc3, 0xd4, 0xfa, 0x17, 0x6b, 0x10, 0xee, 0xd5, 0x62, 0x30, 0xce, 0xc1,
  0xda, 0x99, 0x16, 0xca, 0xcd, 0xf0, 0xc3, 0x95, 0x36, 0x8d, 0xca, 0xa4,
  0xdb, 0xb1, 0x19, 0x93, 0xec, 0xc1, 0x30, 0xe2, 0xbf, 0x2e, 0x65, 0x22,
  0x9f, 0x66, 0xfb, 0x4a, 0xb1, 0xcc, 0x69, 0xae, 0x93, 0x54, 0x82, 0x43,
  0x35, 0xb4, 0xcf, 0xa2, 0x46, 0xf1, 0x0a, 0xb5, 0xb6, 0xee, 0xf3, 0xd0,
  0x36, 0x61, 0xaa, 0xcb, 0x67, 0xb3, 0x14, 0x4c, 0x05, 0x73, 0x2d, 0x10,
  0x07, 0x29, 0xa7, 0xe2, 0xde, 0xe6, 0x1a, 0xd4, 0x68, 0x66, 0x68, 0x2f,
  0x92, 0xf0, 0xde, 0x09, 0x29, 0xfe, 0x61, 0xa1, 0x36, 0x07, 0x52, 0xb1,
  0x1d, 0xc1, 0xe7, 0x74, 0x2c, 0x67, 0xaf, 0x13, 0x62, 0x61, 0x22, 0x54,
  0x4b, 0x22, 0xaf, 0xd7, 0xec, 0xac, 0x82, 0x55, 0x80, 0x9c, 0x2a, 0xd4,
  0x2b, 0x56, 0xc7, 0xf7, 0x69, 0xe7, 0x66, 0x7a, 0x96, 0xac, 0x5f, 0x89,
  0x48, 0xcc, 0xe7, 0xd3, 0x09, 0xae, 0x2a, 0xbc, 0xb7, 0xad, 0x32, 0xfd,
  0x01, 0x6d, 0x0a, 0xd7, 0xbe, 0xd2, 0xdb, 0x49, 0xea, 0x21, 0x49, 0x20,
  0xf3, 0xa3, 0x6d, 0xc4, 0xce, 0xbd, 0x01, 0x37, 0xe0, 0xde, 0xae, 0xf5,
  0x31, 0x82, 0x1f, 0x7b, 0x2f, 0x10, 0xdc, 0x95, 0xbe, 0x02, 0xeb, 0x80,
  0xf2, 0xe9, 0x49, 0xca, 0x6f, 0xb4, 0xab, 0x10, 0x0e, 0x63, 0x8e, 0x21,
  0x98, 0xfc, 0x79, 0x5d, 0xa9, 0x2d, 0xc5, 0xf0, 0xe6, 0x37, 0xbb, 0xf2,
  0x05, 0x1c, 0x7f, 0x2a, 0x48, 0x3d, 0x30, 0x99, 0xf9, 0xf8, 0x9b, 0x0c,
  0x28, 0xf1, 0x8b, 0x78, 0xe3, 0x22, 0x87, 0x59, 0x3c, 0x69, 0xc5, 0x63,
  0x8c, 0x96, 0xff, 0x4a, 0x2f, 0x16, 0x12, 0xe6, 0xb9, 0x87, 0x77, 0x2e,
  0x16, 0xf6, 0x7d, 0x49, 0x0f, 0x5b, 0x02, 0x5f, 0x22, 0x22, 0x8a, 0xd3,
  0x55, 0xc3, 0x6c, 0xa5, 0x40, 0x5e, 0x04, 0x85, 0x51, 0x39, 0x95, 0x9d,
  0xc0, 0x9e, 0x23, 0x1f, 0xa7, 0x68, 0x9b, 0xd6, 0xea, 0xe8, 0x49, 0x35,
  0x65, 0xf3, 0x7b, 0x55, 0xc2, 0xcb, 0xab, 0x06, 0x3d, 0x2e, 0x7f, 0xef,
  0x77, 0x2f, 0x3f, 0xfe, 0xd6, 0xbd, 0xec, 0xf6, 0x7b, 0x6d, 0x73, 0xef,
  0xcb, 0x8a, 0x8e, 0xbf, 0x30, 0x07, 0x2b, 0xf6, 0x74, 0x12, 0xc6, 0x12,
  0x17, 0xfe, 0xd7, 0x06, 0xd7, 0xd5, 0x87, 0xb6, 0xc0, 0x6e, 0x99, 0x5d,
  0x36, 0x6e, 0x26, 0xc0, 0x25, 0xf9, 0xd0, 0xed, 0xfa, 0xa7, 0xe5, 0x0c,
  0x97, 0x44, 0x08, 0xe4, 0x1b, 0x84, 0xaf, 0x5d, 0x14, 0x1e, 0x79, 0x49,
  0x2a, 0x9f, 0xb3, 0xce, 0xa3, 0x7f, 0x75, 0xd5, 0x2d, 0xaf, 0xde, 0x49,
  0xf9, 0x5e, 0x45, 0x5d, 0xec, 0x26, 0xca, 0xbd, 0x45, 0xc2, 0x38, 0xee,
  0x26, 0xde, 0xb0, 0x97, 0xb8, 0xfd, 0xfb, 0xee, 0xee, 0x2c, 0xc5, 0x32,
  0xf9, 0xe1, 0xdc, 0x34, 0x9d, 0x28, 0x4b, 0xc7, 0x9f, 0x8d, 0x5e, 0xe2,
  0x22, 0x98, 0xce, 0xc8, 0x05, 0x99, 0x7c, 0x9d, 0xb7, 0x4e, 0xd6, 0x4a,
  0x38, 0x3f, 0xf4, 0x0a, 0xae, 0xcf, 0xde, 0x5f, 0xca, 0x8b, 0x41, 0xab,
  0x7c, 0xf5, 0x3e, 0xef, 0xb7, 0xad, 0xfc, 0x4d, 0x6d, 0xbe, 0x91, 0xd2,
  0x91, 0xe2, 0xbf, 0xb7, 0x21, 0x2d, 0x59, 0x6e, 0x11, 0xd5, 0x89, 0x0a,
  0x96, 0xb4, 0xe5, 0x77, 0xbe, 0xce, 0x7f, 0xb4, 0x1b, 0xf4, 0xeb, 0xa5,
  0xbf, 0x21, 0x02, 0x25, 0xcd, 0x63, 0x3b, 0x01, 0x8b, 0x13, 0xa8, 0xc5,
  0x81, 0xae, 0x70, 0x51, 0x9e, 0x3d, 0xfc, 0x63, 0x7e, 0x18, 0x79, 0xeb,
  0x69, 0x07, 0x56, 0x9d, 0x48, 0xf8, 0xb1, 0x4f, 0x2e, 0x63, 0x5e, 0xeb,
  0xf5, 0xaf, 0xf5, 0x7f, 0xda, 0xd7, 0x82, 0x1b, 0x61, 0x92, 0x15, 0x33,
  0x70, 0xe0, 0x1c, 0x84, 0xa5, 0x5e, 0x42, 0x11, 0x16, 0x0c, 0x07, 0xb8,
  0x58, 0x23, 0xc1, 0xd9, 0x5f, 0xf3, 0x3b, 0x64, 0xcf, 0x9d, 0xd0, 0x6a,
  0x44, 0x2f, 0xb2, 0x34, 0xc4, 0x92, 0x48, 0x09, 0x28, 0x5e, 0x44, 0x28,
  0xc9, 0xa4, 0x13, 0x29, 0x33, 0x2e, 0x47, 0xdf, 0xc1, 0x51, 0x76, 0xf0,
  0xa8, 0x54, 0x44, 0x3f, 0x3f, 0x63, 0x6d, 0x90, 0x4c, 0xbd, 0xe5, 0xc6,
  0xa3, 0xdd, 0x34, 0xb0, 0xd2, 0x7f, 0xe1, 0xbc, 0xe0, 0xf1, 0xe2, 0x72,
  0x0e, 0x32, 0xe7, 0xb4, 0xda, 0x7c, 0x6f, 0xb3, 0x20, 0x11, 0x6e, 0xbb,
  0x8f, 0x0b, 0x9c, 0x22, 0x78, 0x55, 0x4e, 0x92, 0xb5, 0x68, 0x09, 0x65,
  0x1d, 0x93, 0x32, 0x8f, 0x58, 0xe7, 0x01, 0xb5, 0x42, 0x9a, 0xd8, 0x95,
  0x72, 0x2b, 0x33, 0x24, 0x12, 0xd8, 0x1d, 0x85, 0xd1, 0xdb, 0x88, 0x15,
  0xd3, 0x3d, 0x8b, 0x57, 0x1c, 0x1f, 0x9b, 0x74, 0x8b, 0xb4, 0x76, 0xb9,
  0x6e, 0x3b, 0x08, 0x8b, 0x97, 0xbc, 0x31, 0x4b, 0xc1, 0x97, 0x88, 0x42,
  0xb3, 0x70, 0x02, 0x11, 0xc3, 0x30, 0xda, 0x77, 0xef, 0x6b, 0xd0, 0x43,
  0xdf, 0xbd, 0x1b, 0x77, 0xee, 0x06, 0x30, 0x69, 0x3a, 0x11, 0x2a, 0xa3,
  0x3d, 0xb5, 0x6b, 0x66, 0x0c, 0x2c, 0x10, 0xf2, 0xb5, 0x56, 0x91, 0xf0,
  0x07, 0xa7, 0x67, 0xd9, 0xd7, 0x3f, 0x03, 0x23, 0x38, 0xdb, 0x27, 0x41,
  0x8e, 0x40, 0x33, 0x10, 0x20, 0x87, 0x06, 0x50, 0xa8, 0x9c, 0x12, 0x6a,
  0x71, 0x00, 0x95, 0xc3, 0x64, 0xa0, 0xe3, 0x6f, 0x20, 0x94, 0xe0, 0x82,
  0x99, 0x97, 0x43, 0x97, 0xbf, 0xfc, 0x0f, 0xa0, 0x71, 0x48, 0x86, 0xfe,
  0x0f, 0x00, 0x00
};
unsigned int node_html_len = 1047;
