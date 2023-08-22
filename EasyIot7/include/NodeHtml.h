#include <Arduino.h>
unsigned char node_html[] PROGMEM = {
  0x1f, 0x8b, 0x08, 0x08, 0x78, 0x1e, 0xe5, 0x64, 0x00, 0x03, 0x6e, 0x6f,
  0x64, 0x65, 0x2e, 0x6d, 0x69, 0x6e, 0x2e, 0x68, 0x74, 0x6d, 0x6c, 0x00,
  0xad, 0x57, 0xdd, 0x72, 0xe2, 0x36, 0x14, 0x7e, 0x15, 0x8d, 0xae, 0x76,
  0x2f, 0x0c, 0x04, 0x36, 0x99, 0xb6, 0x03, 0xcc, 0x6c, 0xc3, 0x64, 0xcb,
  0x45, 0xb6, 0x74, 0x49, 0xa6, 0xd7, 0xb2, 0x7c, 0xc0, 0x1a, 0x64, 0xc9,
  0x2b, 0xcb, 0x21, 0xe9, 0x6b, 0xf4, 0x09, 0x76, 0x7a, 0xb1, 0x0f, 0xc2,
  0x8b, 0xf5, 0x48, 0xc6, 0xc4, 0x60, 0x93, 0xe0, 0x84, 0x0b, 0x7b, 0xec,
  0x23, 0xfb, 0xe8, 0xfb, 0x39, 0xfa, 0x1b, 0x46, 0xe2, 0x81, 0x70, 0xc9,
  0xb2, 0x6c, 0x44, 0x43, 0xfd, 0x48, 0xf0, 0x0a, 0x52, 0x23, 0x12, 0x66,
  0x9e, 0xe8, 0x78, 0xb8, 0xdf, 0x18, 0xc4, 0xc0, 0x22, 0x30, 0x18, 0x8f,
  0x07, 0xd5, 0xb0, 0x15, 0x56, 0x02, 0x46, 0xb3, 0x94, 0xa9, 0x32, 0x2e,
  0x99, 0x5a, 0x06, 0x4a, 0x47, 0x18, 0xff, 0xba, 0xf9, 0x77, 0xd8, 0x75,
  0x6d, 0xe3, 0x61, 0x37, 0x1e, 0xe0, 0x0d, 0xb3, 0x8e, 0x87, 0x0b, 0x6d,
  0x12, 0xbc, 0x0b, 0x90, 0x51, 0x06, 0x96, 0x88, 0x68, 0x44, 0x17, 0x0b,
  0x4a, 0x22, 0x91, 0xb1, 0x50, 0x02, 0xbe, 0x95, 0x4f, 0x75, 0x18, 0xa1,
  0x8e, 0x0e, 0xc0, 0xb9, 0x64, 0xc1, 0xd2, 0xe8, 0x3c, 0xc5, 0xb8, 0x64,
  0x21, 0x48, 0x82, 0xa1, 0x11, 0x75, 0x00, 0xa6, 0x51, 0x23, 0x34, 0x96,
  0x38, 0x68, 0x3a, 0x81, 0x1d, 0x36, 0xff, 0xdf, 0x98, 0x0c, 0x85, 0x4a,
  0x73, 0xbb, 0x97, 0x99, 0x6b, 0x65, 0x8d, 0x96, 0x94, 0x24, 0xec, 0x51,
  0x82, 0x5a, 0xda, 0x78, 0x44, 0x07, 0x3d, 0x4a, 0xec, 0x53, 0x0a, 0x23,
  0x6a, 0xe1, 0xd1, 0x52, 0x4f, 0x60, 0xdb, 0x1d, 0x49, 0x25, 0xe3, 0x10,
  0x6b, 0x89, 0x62, 0x8d, 0x28, 0x3c, 0xfe, 0x46, 0x56, 0xc2, 0xf2, 0x18,
  0x14, 0x25, 0x06, 0xbe, 0xe7, 0xc2, 0x40, 0x54, 0xaa, 0x50, 0xdc, 0x9b,
  0xf9, 0x79, 0xcc, 0x2e, 0xad, 0x0c, 0x65, 0xc0, 0x63, 0x91, 0xd2, 0xf2,
  0x23, 0xff, 0x32, 0xee, 0x74, 0x3a, 0x3b, 0xec, 0x4d, 0x69, 0xda, 0xba,
  0x95, 0x01, 0xcf, 0x8d, 0xb0, 0xd8, 0xf5, 0x1c, 0x96, 0xb9, 0x61, 0x6a,
  0xf3, 0x93, 0x35, 0x18, 0xf7, 0x66, 0x33, 0x18, 0xe7, 0x90, 0x65, 0x33,
  0x2d, 0x94, 0x9d, 0xe1, 0x87, 0x6b, 0x6d, 0x1a, 0x9d, 0x49, 0x77, 0x6d,
  0x33, 0x26, 0xd9, 0x83, 0x61, 0xc4, 0x7d, 0x5d, 0xda, 0x44, 0x3e, 0xcf,
  0x0e, 0x9d, 0x62, 0xb9, 0xd5, 0x5c, 0x27, 0xa9, 0x04, 0x8b, 0x6e, 0x68,
  0x57, 0x45, 0x8d, 0xe6, 0x15, 0x6e, 0xed, 0xd2, 0x7b, 0x69, 0x9b, 0x30,
  0xd5, 0xed, 0xcb, 0xf2, 0x14, 0x4c, 0x05, 0x73, 0x4d, 0x88, 0xa3, 0x94,
  0x53, 0x71, 0x9f, 0x79, 0x0f, 0x6a, 0x34, 0x73, 0x8c, 0x17, 0x45, 0x78,
  0x6f, 0x85, 0x14, 0xff, 0xb0, 0x48, 0x9b, 0x23, 0xa5, 0xd8, 0x8e, 0xe0,
  0x73, 0x39, 0x96, 0xbd, 0xd7, 0x09, 0xb1, 0x28, 0x11, 0xaa, 0x25, 0x91,
  0xb7, 0x7b, 0x76, 0x56, 0xc3, 0x2a, 0x40, 0x4e, 0x35, 0xea, 0x0d, 0xa3,
  0xe3, 0xef, 0x69, 0x70, 0x33, 0x3d, 0x4b, 0xd5, 0xaf, 0xc5, 0x42, 0xcc,
  0xe7, 0xd3, 0x09, 0x8e, 0x2a, 0xbc, 0xb7, 0x9d, 0x65, 0xfa, 0x03, 0xda,
  0x24, 0xd7, 0xa1, 0xd3, 0xbb, 0x4e, 0xea, 0x92, 0x24, 0x90, 0xbb, 0xd6,
  0x36, 0x66, 0xfb, 0x6c, 0xc0, 0x0d, 0xd8, 0xf7, 0x7b, 0xdd, 0x48, 0xd0,
  0xd5, 0xfd, 0x5e, 0x37, 0x55, 0xca, 0x57, 0xbd, 0x17, 0x28, 0xef, 0x17,
  0x43, 0x35, 0x43, 0x73, 0x2d, 0xa4, 0x27, 0xd5, 0xc2, 0xd6, 0xcd, 0x8a,
  0x04, 0x51, 0xcc, 0x51, 0x94, 0xc9, 0x1f, 0xd7, 0x95, 0xd9, 0xa6, 0x68,
  0xde, 0xfe, 0x96, 0xad, 0xdd, 0x94, 0x8e, 0x3f, 0x15, 0x34, 0x1f, 0x98,
  0xcc, 0x9d, 0x23, 0x26, 0x87, 0x92, 0x9e, 0x4f, 0xe1, 0x61, 0x16, 0x4f,
  0x5a, 0xf1, 0x18, 0xf5, 0x73, 0x5f, 0xe9, 0xe5, 0x52, 0xc2, 0xdc, 0x67,
  0xf8, 0x60, 0x63, 0x91, 0x7d, 0x2c, 0xe9, 0xe1, 0x22, 0xc1, 0x57, 0x88,
  0x88, 0x62, 0x77, 0x55, 0xe1, 0x33, 0x29, 0x90, 0x17, 0x41, 0xab, 0x94,
  0xa7, 0xb2, 0x27, 0xf5, 0x39, 0x2a, 0x74, 0x8a, 0xb1, 0x69, 0x6d, 0x66,
  0x3d, 0x69, 0x96, 0xd9, 0xfe, 0x5e, 0xb5, 0xf0, 0xe2, 0xb2, 0xc1, 0x8f,
  0x8b, 0x5f, 0xfb, 0x9d, 0x8b, 0xab, 0x5f, 0x3a, 0x17, 0x9d, 0x7e, 0xaf,
  0x6d, 0x35, 0x7e, 0x59, 0xd3, 0xf1, 0x17, 0x66, 0x61, 0xcd, 0x9e, 0x4e,
  0xc2, 0x58, 0xe2, 0xc2, 0xff, 0xda, 0xe0, 0xba, 0xfc, 0xd4, 0x16, 0xd8,
  0x2d, 0xcb, 0x56, 0x8d, 0xdb, 0x0b, 0xb0, 0x89, 0x6f, 0xba, 0xdd, 0xfc,
  0xc8, 0x38, 0xc3, 0x41, 0x12, 0x01, 0xf9, 0x06, 0xd1, 0x5b, 0x77, 0x1b,
  0x0e, 0x79, 0x49, 0xca, 0xf7, 0x59, 0xe7, 0xd1, 0xbf, 0xbc, 0xec, 0x94,
  0x57, 0xef, 0xa4, 0x7a, 0xaf, 0xa2, 0x2e, 0xf6, 0x17, 0xe5, 0x6e, 0x23,
  0x61, 0x1c, 0xf7, 0x17, 0xef, 0xd8, 0x5d, 0xdc, 0xfe, 0x75, 0x77, 0x77,
  0x96, 0xe9, 0x33, 0xf9, 0x6e, 0xed, 0x34, 0x9d, 0xa8, 0x8c, 0x8e, 0x7f,
  0x37, 0x7a, 0x85, 0x83, 0x60, 0x3a, 0x23, 0x5d, 0x32, 0xf9, 0x3a, 0x6f,
  0x5d, 0xac, 0x15, 0x39, 0x3f, 0xf5, 0x0a, 0xae, 0xcf, 0xd9, 0x5f, 0xaa,
  0x8b, 0x41, 0xab, 0x7a, 0x75, 0x39, 0xef, 0x77, 0x8b, 0xfb, 0xbb, 0x16,
  0xfe, 0x46, 0x4a, 0xaf, 0x2c, 0x07, 0x07, 0x5b, 0xd4, 0x92, 0xe5, 0x0e,
  0x51, 0x9d, 0xa8, 0x60, 0x49, 0x5b, 0x7e, 0xe7, 0xdb, 0x0b, 0xbc, 0x56,
  0xf8, 0x57, 0xfd, 0xfa, 0xd4, 0xdf, 0xa0, 0x40, 0x49, 0xf3, 0xb5, 0xbd,
  0x41, 0x86, 0x1d, 0xa8, 0xe5, 0x91, 0x55, 0xa1, 0x5b, 0x9e, 0x46, 0xdc,
  0xa3, 0x3f, 0x9e, 0xbc, 0xf7, 0xfc, 0x03, 0xeb, 0x60, 0x21, 0x5c, 0xdb,
  0x67, 0x9b, 0x33, 0xe7, 0xf5, 0xe6, 0xe7, 0xe6, 0x3f, 0xed, 0xe6, 0x82,
  0x1b, 0x61, 0x92, 0x35, 0x33, 0x70, 0xe4, 0x64, 0x84, 0x53, 0xbd, 0x84,
  0x42, 0x16, 0x94, 0x03, 0x6c, 0xac, 0x91, 0xe0, 0xec, 0xcf, 0xf9, 0x1d,
  0xb2, 0xe7, 0x56, 0x68, 0x35, 0xa2, 0xdd, 0x3c, 0x8d, 0x70, 0x4a, 0xa4,
  0x04, 0x14, 0x2f, 0x14, 0x4a, 0x72, 0x69, 0x45, 0xca, 0x8c, 0xf5, 0xe8,
  0x03, 0x6c, 0x65, 0x47, 0x0f, 0x4f, 0x85, 0xfa, 0xfe, 0xd4, 0xb5, 0x45,
  0x32, 0x75, 0x91, 0x1b, 0x87, 0x76, 0xbb, 0x80, 0x95, 0xf9, 0x8b, 0xe4,
  0x05, 0x8f, 0x17, 0x87, 0x73, 0x98, 0x5b, 0xab, 0xd5, 0xf6, 0xfb, 0x2c,
  0x0f, 0x13, 0x61, 0x77, 0x3b, 0xbb, 0xd0, 0x2a, 0x82, 0x57, 0xe5, 0x6c,
  0x59, 0x53, 0x4b, 0xa8, 0xcc, 0x32, 0x29, 0xbd, 0x62, 0xc1, 0x03, 0x7a,
  0x85, 0x34, 0x71, 0x55, 0xf2, 0x51, 0x66, 0xc8, 0x42, 0xe0, 0xea, 0x28,
  0x8c, 0xde, 0x29, 0x56, 0x74, 0xf7, 0x6c, 0x5e, 0x93, 0x63, 0x0b, 0xad,
  0xad, 0x77, 0xac, 0x68, 0xdd, 0x43, 0x58, 0xbc, 0xf8, 0x85, 0x59, 0x0a,
  0xbe, 0x42, 0x14, 0x9a, 0x45, 0x13, 0x58, 0x30, 0x94, 0x31, 0xfb, 0xf0,
  0xb1, 0x06, 0x1d, 0x45, 0x52, 0x42, 0x2d, 0x9b, 0xa0, 0x1b, 0xc0, 0xaa,
  0x09, 0x16, 0x68, 0x8d, 0x76, 0xdc, 0xae, 0x99, 0x31, 0xb0, 0x44, 0xcc,
  0xd7, 0x5a, 0x2d, 0x84, 0x3b, 0x4b, 0x3d, 0xfb, 0xbe, 0xf9, 0x11, 0x1a,
  0xc1, 0xd9, 0x21, 0x0b, 0xf2, 0x0a, 0x36, 0x03, 0x21, 0x52, 0x69, 0x40,
  0x15, 0xb9, 0x3d, 0x45, 0xe3, 0x09, 0x03, 0x41, 0x59, 0x2c, 0x06, 0x3a,
  0xfe, 0x06, 0x42, 0x09, 0x2e, 0x98, 0xa9, 0x4b, 0x57, 0xc8, 0x52, 0x19,
  0x04, 0xff, 0x03, 0xcb, 0xef, 0xac, 0x4b, 0x10, 0x10, 0x00, 0x00
};
unsigned int node_html_len = 1055;
