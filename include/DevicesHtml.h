#include <Arduino.h>
unsigned char devices_html[] PROGMEM = {
  0x1f, 0x8b, 0x08, 0x08, 0xfa, 0x07, 0x4c, 0x65, 0x00, 0x03, 0x64, 0x65,
  0x76, 0x69, 0x63, 0x65, 0x73, 0x2e, 0x6d, 0x69, 0x6e, 0x2e, 0x68, 0x74,
  0x6d, 0x6c, 0x00, 0x9d, 0x55, 0xdb, 0x6e, 0xdb, 0x30, 0x0c, 0xfd, 0x15,
  0x4d, 0x0f, 0xc3, 0x06, 0xcc, 0x89, 0xbb, 0xbc, 0x0c, 0x5d, 0x92, 0xa2,
  0x6b, 0xbb, 0x62, 0xc0, 0xd6, 0x15, 0x4d, 0xba, 0xd7, 0x41, 0x96, 0x18,
  0x5b, 0xa8, 0x2c, 0x79, 0x92, 0x1c, 0xa7, 0xdf, 0xb3, 0x87, 0x7d, 0x48,
  0x7f, 0x6c, 0xf4, 0xb5, 0x8e, 0xeb, 0x74, 0x17, 0x20, 0x68, 0x4a, 0x89,
  0xe4, 0x39, 0x3c, 0xa4, 0x98, 0xb9, 0x90, 0x5b, 0xc2, 0x15, 0x73, 0x6e,
  0x41, 0x23, 0xb3, 0x0b, 0x22, 0x23, 0xee, 0xe9, 0x72, 0xde, 0x3b, 0xdd,
  0x00, 0xf3, 0xb9, 0x05, 0x47, 0x89, 0x14, 0x0b, 0x2a, 0x60, 0x2b, 0x39,
  0xb8, 0xef, 0xdc, 0xe8, 0x8d, 0x8c, 0xd1, 0xd3, 0x43, 0x9a, 0x29, 0xe6,
  0xa1, 0xba, 0x3d, 0x3d, 0x5b, 0xdf, 0x9e, 0xae, 0xbf, 0xde, 0xd4, 0x19,
  0x96, 0x73, 0xb7, 0x8d, 0x49, 0x21, 0x85, 0x4f, 0x16, 0x74, 0x16, 0x66,
  0x3b, 0x4a, 0x12, 0x90, 0x71, 0xe2, 0x5b, 0x6b, 0x97, 0x2a, 0x8d, 0x08,
  0x89, 0xf7, 0xd9, 0xf1, 0x74, 0x5a, 0x14, 0xc5, 0xa4, 0x98, 0x4d, 0x8c,
  0x8d, 0xa7, 0x6f, 0xc3, 0x30, 0x9c, 0x62, 0x30, 0x25, 0x5b, 0x09, 0xc5,
  0x07, 0xb3, 0x5b, 0xd0, 0x90, 0x84, 0x64, 0x56, 0x7e, 0x30, 0x77, 0x79,
  0x35, 0xc6, 0x11, 0xd9, 0xef, 0xe8, 0x92, 0xcc, 0x4f, 0x30, 0x31, 0xd9,
  0x82, 0x75, 0xd2, 0xe8, 0x05, 0x3d, 0x9a, 0x84, 0x94, 0x80, 0xe6, 0x46,
  0x48, 0x1d, 0x2f, 0x68, 0xee, 0x37, 0xc1, 0x3b, 0x7a, 0xb2, 0x9c, 0xbf,
  0x08, 0x02, 0x72, 0x9b, 0x29, 0xc3, 0x04, 0x08, 0xe2, 0xcd, 0x31, 0x59,
  0x7d, 0xbb, 0x24, 0x37, 0x90, 0x99, 0x37, 0xa4, 0xa4, 0x82, 0x18, 0x16,
  0x8d, 0x09, 0x37, 0xe9, 0x1b, 0x72, 0x09, 0x1a, 0x2c, 0xf3, 0xc6, 0x3e,
  0x7a, 0x91, 0x2f, 0x72, 0x07, 0x96, 0xac, 0x8d, 0x51, 0x8e, 0x04, 0x01,
  0xc2, 0x2a, 0x16, 0x81, 0xc2, 0xa2, 0x33, 0xa6, 0x87, 0xc4, 0x34, 0x4b,
  0xa1, 0x22, 0x8e, 0x77, 0xf8, 0x55, 0x7b, 0x36, 0x11, 0xad, 0xaf, 0x2b,
  0xa4, 0xe7, 0x09, 0x7a, 0x49, 0x9d, 0xe5, 0x9e, 0x18, 0xcd, 0x13, 0xa6,
  0x63, 0x58, 0x50, 0x6f, 0xe2, 0x58, 0xc1, 0xaa, 0xba, 0x7e, 0xe5, 0x13,
  0xe9, 0x5e, 0x53, 0xe2, 0xef, 0x33, 0xbc, 0xe1, 0x09, 0xf0, 0xbb, 0xa6,
  0xe8, 0x3e, 0xac, 0x53, 0x52, 0x20, 0x37, 0x6b, 0x72, 0x2d, 0x46, 0x60,
  0x87, 0x00, 0x2e, 0xc9, 0xbd, 0x07, 0x7b, 0x0d, 0x96, 0x83, 0xf6, 0x2c,
  0x86, 0x16, 0xa5, 0x4d, 0x57, 0xdf, 0x07, 0x75, 0x5a, 0x4a, 0x52, 0x89,
  0xb2, 0xa2, 0xa8, 0x29, 0xc3, 0xc6, 0x1c, 0x85, 0x61, 0x4b, 0xc7, 0x96,
  0xe9, 0x4a, 0xbc, 0xaa, 0xf9, 0xcd, 0xdf, 0x76, 0x40, 0x06, 0xa3, 0xb2,
  0xba, 0xb8, 0x5a, 0xb5, 0x83, 0x32, 0xda, 0xc6, 0x6a, 0x78, 0x0e, 0xb7,
  0xff, 0xdf, 0xe5, 0x7e, 0x4a, 0xa8, 0xcf, 0xb3, 0xa4, 0x51, 0xf2, 0x4a,
  0x8d, 0x60, 0xaa, 0xab, 0xbc, 0xb6, 0xf6, 0x48, 0x56, 0x47, 0x01, 0xce,
  0xbf, 0x47, 0xad, 0xc6, 0xae, 0x12, 0x60, 0xa5, 0x4a, 0xfb, 0xcc, 0xb8,
  0x32, 0x0e, 0x29, 0xbd, 0xf4, 0x32, 0x05, 0xf7, 0xbe, 0x65, 0x96, 0xcc,
  0x3a, 0xe6, 0x1d, 0xe7, 0x64, 0xd6, 0xa7, 0xb4, 0x97, 0x79, 0x63, 0x6c,
  0x8a, 0x2e, 0xf5, 0xd4, 0xa0, 0xd1, 0x0b, 0xeb, 0x63, 0x29, 0x6c, 0x43,
  0x73, 0x7e, 0x65, 0x52, 0x38, 0xd0, 0xfe, 0x3d, 0xe0, 0x32, 0x5b, 0x5a,
  0x15, 0x65, 0x8d, 0xaa, 0x1a, 0xab, 0x40, 0xc7, 0xf5, 0xbb, 0x6d, 0xbb,
  0xeb, 0x61, 0xe7, 0x29, 0x29, 0xbd, 0x3b, 0xdc, 0x4a, 0xb1, 0xf6, 0x7f,
  0x14, 0x95, 0x43, 0x62, 0x14, 0x16, 0xbf, 0xa0, 0xb0, 0x3b, 0x26, 0x77,
  0xe5, 0xc0, 0x82, 0xa6, 0xc4, 0xc2, 0x8f, 0x5c, 0x5a, 0x10, 0x23, 0x85,
  0x6d, 0x02, 0xc6, 0xc9, 0xd3, 0xea, 0x96, 0x6b, 0x89, 0x2f, 0x4c, 0x00,
  0xf9, 0x84, 0x32, 0x5b, 0x9b, 0x67, 0xf8, 0xf4, 0xba, 0x3e, 0x8e, 0xea,
  0x12, 0x58, 0x53, 0xb4, 0x6b, 0xa7, 0x2e, 0xb0, 0x9d, 0x49, 0x21, 0x4d,
  0x4b, 0x54, 0xea, 0x00, 0x23, 0x20, 0xc8, 0x72, 0x97, 0x3c, 0x96, 0xd2,
  0x9c, 0xe2, 0xb2, 0x61, 0x2a, 0x87, 0x72, 0xb2, 0xbb, 0xa7, 0xd9, 0x88,
  0xbc, 0x17, 0xb7, 0xbc, 0xce, 0x95, 0x63, 0xa2, 0x47, 0xa8, 0xab, 0xea,
  0x2f, 0xa0, 0x71, 0xf2, 0xf8, 0x33, 0xd8, 0x47, 0x07, 0xb1, 0xeb, 0x40,
  0xec, 0xa8, 0x4d, 0x99, 0x1a, 0xce, 0xf4, 0xc8, 0x20, 0x6f, 0x02, 0xce,
  0x94, 0x8c, 0x70, 0x6b, 0xe1, 0x16, 0xa4, 0x7b, 0x7a, 0x27, 0xf8, 0x8c,
  0xc7, 0x44, 0x3f, 0xab, 0x03, 0x1e, 0x7e, 0x3d, 0xfc, 0x34, 0xff, 0xa7,
  0xb6, 0xce, 0xd3, 0xa8, 0xdc, 0x10, 0x35, 0x81, 0x3c, 0x1b, 0xd3, 0xb4,
  0x9b, 0x52, 0x34, 0x82, 0x44, 0xe2, 0x4c, 0x35, 0x95, 0xa2, 0xfb, 0xd2,
  0x41, 0x8c, 0x3b, 0xcb, 0x38, 0x92, 0x31, 0xcb, 0x88, 0xcb, 0x23, 0xf9,
  0x47, 0x99, 0xf7, 0x31, 0x85, 0x29, 0xf4, 0x98, 0x9a, 0x07, 0x51, 0xab,
  0x80, 0x01, 0xae, 0x00, 0xc7, 0x61, 0x08, 0x3c, 0x14, 0x79, 0x20, 0x8a,
  0xf1, 0xd5, 0xa3, 0x8f, 0x70, 0x55, 0x1a, 0xdd, 0x50, 0xab, 0x0d, 0x5a,
  0xae, 0x5a, 0x25, 0xf9, 0x5d, 0xf9, 0xf3, 0xa9, 0xc0, 0xc3, 0xc7, 0x7a,
  0x4d, 0x0d, 0xb6, 0x6c, 0xe4, 0x75, 0x50, 0xdf, 0xd7, 0xb5, 0xf4, 0xec,
  0xe5, 0xf9, 0xc5, 0xe7, 0x8b, 0xf5, 0xc5, 0x7c, 0x5a, 0x27, 0xc4, 0xa2,
  0x9e, 0x87, 0x61, 0x59, 0xa6, 0xee, 0x1b, 0x94, 0xb3, 0x6a, 0xc9, 0xbb,
  0x11, 0xb0, 0x3c, 0x13, 0xac, 0x0f, 0xd6, 0xd8, 0x23, 0xcb, 0xc4, 0xb1,
  0x2d, 0x9e, 0xdf, 0x5e, 0x9f, 0x9f, 0x96, 0x24, 0x9a, 0x75, 0xd2, 0x70,
  0x79, 0xaa, 0xce, 0x6f, 0xbd, 0xf0, 0x56, 0x42, 0x54, 0x08, 0x00, 0x00
};
unsigned int devices_html_len = 828;
