#include <Arduino.h>
unsigned char styles_min_css[] PROGMEM = {
  0x1f, 0x8b, 0x08, 0x08, 0xb9, 0x55, 0x40, 0x65, 0x00, 0x03, 0x73, 0x74,
  0x79, 0x6c, 0x65, 0x73, 0x2e, 0x6d, 0x69, 0x6e, 0x2e, 0x63, 0x73, 0x73,
  0x00, 0xad, 0x58, 0xcb, 0x6e, 0xe3, 0x36, 0x14, 0xdd, 0xf7, 0x2b, 0xdc,
  0x04, 0x03, 0x4c, 0x5a, 0xcb, 0x90, 0x1d, 0xd9, 0xc9, 0x48, 0xe8, 0x00,
  0xdd, 0x0c, 0xba, 0x6a, 0xb7, 0x2d, 0x8a, 0x59, 0x50, 0x12, 0x65, 0xb3,
  0xa1, 0x49, 0x81, 0xa4, 0x62, 0x7b, 0x04, 0xff, 0x7b, 0x2f, 0x5f, 0x12,
  0xf5, 0x70, 0x9a, 0x01, 0x8a, 0x20, 0x09, 0xcd, 0xc7, 0xe5, 0x7d, 0x9e,
  0x73, 0xe9, 0x9c, 0x97, 0x97, 0xb6, 0xe2, 0x4c, 0x45, 0x27, 0x4c, 0xf6,
  0x07, 0x95, 0x26, 0x71, 0x9c, 0x99, 0xcf, 0x15, 0x3a, 0x12, 0x7a, 0x49,
  0xef, 0x7e, 0xc3, 0xf4, 0x15, 0x2b, 0x52, 0xa0, 0xc5, 0xef, 0xb8, 0xc1,
  0x77, 0xcb, 0xee, 0xf3, 0xf2, 0x57, 0x41, 0x10, 0x5d, 0x4a, 0xc4, 0x64,
  0x24, 0xb1, 0x20, 0x95, 0x3d, 0x27, 0xc9, 0x37, 0x9c, 0xae, 0x93, 0xfa,
  0x9c, 0x15, 0x9c, 0x72, 0x91, 0xde, 0x17, 0x45, 0x91, 0xe5, 0xa8, 0x78,
  0xd9, 0x0b, 0xde, 0xb0, 0x32, 0xbd, 0xdf, 0x6c, 0x36, 0xd9, 0x11, 0x89,
  0x3d, 0x61, 0x69, 0x7c, 0x5d, 0x71, 0x46, 0x09, 0xc3, 0x6d, 0xbf, 0x21,
  0xb2, 0xc7, 0x2a, 0x2e, 0xb0, 0x54, 0x7b, 0x81, 0x31, 0x5b, 0xfc, 0x48,
  0x8e, 0x35, 0x17, 0x0a, 0x31, 0x95, 0xbd, 0xb5, 0x78, 0x5d, 0x51, 0x5c,
  0xb6, 0x07, 0x6b, 0xc7, 0x7a, 0x03, 0x1a, 0x9c, 0x48, 0xa9, 0x0e, 0x76,
  0x98, 0x73, 0x51, 0x62, 0x91, 0xae, 0xf5, 0x70, 0x7c, 0xd7, 0x7d, 0xb5,
  0xdd, 0x66, 0x5a, 0x8d, 0xc8, 0x1f, 0xee, 0x0e, 0x44, 0x02, 0x95, 0xa4,
  0x91, 0xe9, 0x3a, 0x86, 0x29, 0xab, 0x75, 0xa4, 0x78, 0x9d, 0x82, 0xc8,
  0xeb, 0x8a, 0xa1, 0xd7, 0x1c, 0x89, 0xb6, 0x24, 0xb2, 0xa6, 0xe8, 0x92,
  0x56, 0x14, 0x9f, 0xb3, 0x7f, 0x1a, 0xa9, 0x48, 0x75, 0x01, 0xc9, 0x4c,
  0x61, 0xa6, 0x52, 0x59, 0xa3, 0x02, 0x47, 0x39, 0x56, 0x27, 0x50, 0x76,
  0xe6, 0xea, 0x24, 0x2e, 0xab, 0xe7, 0x5d, 0x56, 0x73, 0x49, 0x14, 0xe1,
  0x2c, 0xad, 0xc8, 0x19, 0x97, 0x5e, 0xf1, 0x38, 0xfe, 0x90, 0x7d, 0x8b,
  0x08, 0x2b, 0xf1, 0x39, 0xfd, 0xf4, 0xe9, 0x93, 0xf6, 0x16, 0xaf, 0x04,
  0x8e, 0x8e, 0x98, 0x35, 0xde, 0xd0, 0x64, 0xd7, 0x1b, 0x0a, 0xfb, 0xaf,
  0xab, 0x0a, 0x23, 0xd5, 0x80, 0x7f, 0x86, 0x7a, 0xe9, 0x3f, 0x51, 0x49,
  0x04, 0x2e, 0xcc, 0x35, 0x82, 0x9f, 0xec, 0xd4, 0x49, 0xa0, 0x3a, 0xd5,
  0x7f, 0xb2, 0x3d, 0x0c, 0xb4, 0x99, 0x9d, 0x84, 0x88, 0x80, 0x11, 0x11,
  0xaf, 0xaa, 0xb6, 0x22, 0x94, 0x82, 0x97, 0xaa, 0xca, 0x5d, 0xf4, 0xa8,
  0xbd, 0xe1, 0xee, 0x37, 0xe3, 0x8a, 0x72, 0xa4, 0x52, 0x8a, 0x2b, 0x35,
  0x3e, 0xcd, 0xba, 0xc3, 0x79, 0xb5, 0xbe, 0x75, 0xbe, 0x3f, 0xc4, 0xd0,
  0x11, 0xb7, 0xd6, 0x35, 0xa7, 0x03, 0x51, 0x78, 0x10, 0x96, 0x24, 0x08,
  0x82, 0xbe, 0xcb, 0x46, 0xa5, 0xbf, 0xdb, 0x49, 0xdf, 0x3c, 0x0d, 0x44,
  0xe6, 0xfc, 0x3c, 0x4d, 0xaf, 0xfb, 0xc7, 0xc7, 0x47, 0xb7, 0x1d, 0x32,
  0x7e, 0x3e, 0xdc, 0x6e, 0xca, 0x07, 0xea, 0x29, 0xd9, 0x25, 0x3b, 0x3f,
  0x29, 0xd5, 0x85, 0xe2, 0x54, 0x72, 0x4a, 0x4a, 0x3f, 0xe5, 0x62, 0x00,
  0x07, 0x07, 0x8e, 0xaf, 0x51, 0x59, 0x12, 0xb6, 0xb7, 0x59, 0xe8, 0xf2,
  0xde, 0xe9, 0x3d, 0x8e, 0xc8, 0x75, 0xf5, 0x8a, 0x85, 0x2e, 0x2d, 0x1a,
  0x15, 0x90, 0x3b, 0x58, 0xb4, 0xbe, 0x4e, 0xfa, 0xfc, 0x40, 0x39, 0x5c,
  0xda, 0x80, 0x63, 0x74, 0x16, 0x6e, 0x21, 0x3f, 0xa2, 0xa3, 0x8c, 0x94,
  0x80, 0x22, 0x84, 0xa2, 0x38, 0xa6, 0x66, 0x44, 0x91, 0xc2, 0x7f, 0x7d,
  0x8c, 0x60, 0xf5, 0x21, 0xbb, 0xbd, 0x34, 0xc8, 0xa6, 0xcf, 0x94, 0x7c,
  0x46, 0xad, 0xb3, 0xc4, 0xf8, 0xf6, 0xb1, 0x3e, 0x2f, 0x8c, 0x7d, 0x0b,
  0x73, 0xb0, 0x46, 0x02, 0x77, 0xf5, 0x67, 0x52, 0x21, 0xcc, 0x3f, 0x85,
  0xcf, 0x2a, 0x2a, 0x71, 0xc1, 0x05, 0x32, 0x5a, 0x32, 0xce, 0xf0, 0x58,
  0xfe, 0x0a, 0x81, 0xa1, 0xaf, 0xf8, 0x33, 0x5a, 0x8e, 0x17, 0xf4, 0xff,
  0x88, 0xd7, 0x98, 0x4d, 0xd7, 0xd2, 0x03, 0x07, 0x9f, 0x80, 0x6a, 0xc1,
  0xc5, 0x4e, 0xcb, 0x9c, 0x2b, 0xc5, 0x8f, 0x69, 0xd2, 0xe9, 0x69, 0x62,
  0x3a, 0xaf, 0x89, 0x54, 0x48, 0xc9, 0xa8, 0x46, 0x0c, 0xd3, 0x61, 0x59,
  0x20, 0x4a, 0xf6, 0x2c, 0x82, 0x44, 0x3b, 0xca, 0xd4, 0xfa, 0x3c, 0xac,
  0x3d, 0x5f, 0x12, 0x73, 0xd5, 0x33, 0xaa, 0xd8, 0x00, 0x1b, 0x92, 0xed,
  0x2c, 0xc8, 0x68, 0xed, 0xe6, 0x8a, 0xb9, 0xe2, 0x5c, 0x87, 0x7a, 0x0c,
  0x91, 0x5d, 0xde, 0x6c, 0x7b, 0x28, 0x0d, 0xac, 0xd7, 0x17, 0xad, 0x7b,
  0xd3, 0xcb, 0x4d, 0xb9, 0x2b, 0xc1, 0x50, 0x07, 0x3b, 0xed, 0x11, 0x74,
  0x71, 0x01, 0xda, 0x6c, 0xb5, 0x05, 0x03, 0x71, 0x4e, 0x59, 0x61, 0xd6,
  0x51, 0xa3, 0xf8, 0xa0, 0xaa, 0xcc, 0x84, 0xdb, 0x6f, 0xee, 0x79, 0x36,
  0xf5, 0x14, 0x62, 0x0f, 0x25, 0x52, 0xb9, 0x1a, 0xd0, 0x0e, 0xce, 0xfa,
  0x3c, 0x75, 0xd7, 0xc4, 0xe3, 0xe8, 0xb7, 0xd3, 0x2d, 0x5d, 0x9d, 0x10,
  0xc3, 0x01, 0x51, 0x4e, 0x79, 0xf1, 0x32, 0x93, 0x95, 0xa1, 0x2a, 0xa6,
  0x8a, 0xfc, 0x44, 0x8f, 0x01, 0x7e, 0xc6, 0x5a, 0x64, 0xa6, 0xbc, 0x70,
  0x27, 0x55, 0xa3, 0x40, 0x10, 0x23, 0x0b, 0x74, 0x30, 0x09, 0x5e, 0x42,
  0xe0, 0xcf, 0x36, 0x20, 0xab, 0xe9, 0x49, 0x1f, 0xa3, 0x2e, 0x1f, 0xde,
  0x82, 0x56, 0x90, 0xd4, 0x1c, 0xc1, 0xb9, 0x18, 0x12, 0x57, 0xe2, 0xde,
  0xf1, 0xcf, 0xfe, 0xc6, 0x5c, 0x73, 0x6e, 0x37, 0xfd, 0x5f, 0xe2, 0x74,
  0xae, 0xe9, 0x7b, 0x37, 0x5d, 0x1e, 0x76, 0x90, 0x0d, 0x79, 0x7d, 0x91,
  0x90, 0xba, 0x5e, 0xbd, 0x81, 0x98, 0xe1, 0x99, 0x81, 0xbc, 0xa9, 0x6e,
  0x9a, 0xce, 0x7e, 0x5a, 0xa6, 0xa8, 0x02, 0x31, 0xcb, 0x34, 0xc7, 0x9a,
  0x60, 0x97, 0x84, 0xd5, 0x8d, 0xfa, 0x5b, 0x5d, 0x6a, 0xfc, 0x4b, 0x71,
  0xc0, 0xc5, 0x0b, 0xe8, 0xfe, 0x35, 0x9c, 0xd4, 0x58, 0xc9, 0xbf, 0xb6,
  0xd0, 0x39, 0xe4, 0x2f, 0x44, 0x69, 0x9c, 0xd5, 0xe4, 0xaf, 0xe5, 0x75,
  0x15, 0x7a, 0x06, 0x78, 0xe2, 0xdf, 0xe6, 0x97, 0xae, 0x15, 0xc1, 0xb4,
  0x94, 0x58, 0xb5, 0x7d, 0x46, 0x38, 0x96, 0x8e, 0x33, 0x9d, 0xbf, 0xb6,
  0x10, 0xe3, 0xa0, 0x59, 0xd0, 0x38, 0x66, 0x98, 0x55, 0x70, 0xda, 0x8e,
  0xba, 0x8d, 0x01, 0x7f, 0xaf, 0x92, 0xcd, 0xf3, 0xf6, 0x69, 0x9d, 0x3c,
  0xce, 0x37, 0x21, 0x61, 0x49, 0x0e, 0xa2, 0x1d, 0x16, 0x7f, 0x48, 0x3a,
  0x5e, 0x43, 0xc0, 0xba, 0x45, 0xd8, 0x4e, 0xc4, 0xbe, 0xfe, 0x8c, 0xfc,
  0x01, 0x2e, 0x05, 0xc5, 0xa9, 0x17, 0x83, 0xdc, 0x83, 0x44, 0xc8, 0x06,
  0x4e, 0x3b, 0xa0, 0x92, 0x9f, 0xa0, 0x0a, 0xc0, 0x17, 0x8b, 0x78, 0xa1,
  0x0f, 0xea, 0x5f, 0xb1, 0xcf, 0xd1, 0xc7, 0x78, 0xa9, 0x7f, 0x56, 0xf1,
  0xd3, 0xf6, 0x21, 0xfb, 0x9e, 0xbd, 0x5e, 0xbe, 0xc1, 0x6e, 0x8b, 0x53,
  0x21, 0x9d, 0x2d, 0x56, 0xeb, 0xad, 0x5c, 0x60, 0x24, 0x81, 0xa6, 0x81,
  0xa4, 0x1b, 0xb5, 0x9c, 0x2a, 0x34, 0xd9, 0x93, 0x45, 0xfc, 0xfd, 0xf2,
  0xde, 0x92, 0xf3, 0x7f, 0x08, 0x01, 0xa8, 0x3b, 0x90, 0xfa, 0x06, 0xb1,
  0x87, 0x0d, 0x84, 0x47, 0xc1, 0x00, 0x38, 0x74, 0xf7, 0xb6, 0xd0, 0x85,
  0xb7, 0xf0, 0x83, 0x11, 0xfd, 0x6f, 0x4d, 0xa9, 0x2a, 0xd6, 0xde, 0x88,
  0x7b, 0xd7, 0x26, 0x68, 0xee, 0x1b, 0xe5, 0xe1, 0xb8, 0x99, 0x36, 0x84,
  0x64, 0x68, 0xc6, 0x13, 0x4c, 0xd1, 0x08, 0x09, 0xea, 0xd5, 0x9c, 0x58,
  0xbe, 0xd1, 0x6a, 0x46, 0xa6, 0x4f, 0x04, 0x30, 0x35, 0xad, 0x98, 0xe1,
  0x74, 0xde, 0x14, 0x87, 0x08, 0xd9, 0xa2, 0x3d, 0x22, 0x46, 0xea, 0x86,
  0x1a, 0x4a, 0xcb, 0x6e, 0xaf, 0xf8, 0x20, 0x36, 0x52, 0xb7, 0x28, 0x98,
  0x42, 0xc9, 0x5b, 0x7c, 0x36, 0x55, 0x38, 0x33, 0x2b, 0xa7, 0x93, 0x93,
  0x89, 0xbe, 0x73, 0x9e, 0xf6, 0x03, 0xc6, 0x4b, 0x96, 0xa3, 0x5b, 0x0e,
  0x16, 0x10, 0x75, 0x01, 0xb6, 0xf8, 0x60, 0xa6, 0xa3, 0x5a, 0x10, 0xc8,
  0xfa, 0xcb, 0x80, 0xba, 0x27, 0xe1, 0x5a, 0x43, 0x5b, 0xb5, 0x29, 0x46,
  0xbd, 0xd6, 0x66, 0xa7, 0x7f, 0xae, 0x1d, 0xeb, 0xbb, 0x4d, 0xf3, 0xdc,
  0xfe, 0xc7, 0xef, 0x33, 0x59, 0x60, 0xdb, 0x6a, 0x58, 0xfc, 0xf2, 0x65,
  0x66, 0xd5, 0xca, 0xb3, 0x5a, 0x9e, 0x90, 0x60, 0x10, 0xe2, 0xb7, 0xb5,
  0x84, 0xee, 0xf5, 0x29, 0xc9, 0x47, 0x5a, 0xda, 0x49, 0x2b, 0xa5, 0x44,
  0x6c, 0xdf, 0xb3, 0xc8, 0x0d, 0x21, 0xf0, 0xca, 0x18, 0x49, 0xd8, 0x6e,
  0xf5, 0x71, 0x1a, 0x49, 0xf4, 0x8a, 0x21, 0x49, 0x68, 0xeb, 0x5f, 0x2c,
  0x7d, 0xa7, 0x6b, 0xe0, 0x62, 0x40, 0xd8, 0x3a, 0xeb, 0x00, 0xfa, 0x4f,
  0x44, 0x15, 0x87, 0xb6, 0x6b, 0x43, 0x04, 0xd6, 0x49, 0xf0, 0x8a, 0x67,
  0x59, 0xd5, 0xe1, 0xda, 0x2e, 0xec, 0xbb, 0x93, 0x5e, 0xca, 0xc2, 0xa0,
  0x7a, 0x17, 0xc1, 0x38, 0xf3, 0xd0, 0xeb, 0xf6, 0x02, 0xf4, 0x4a, 0x88,
  0x3c, 0x18, 0x38, 0xed, 0x43, 0x47, 0xd9, 0xac, 0x01, 0x2e, 0xce, 0x0c,
  0x33, 0xc7, 0x99, 0xd5, 0x57, 0xc3, 0xba, 0x81, 0xc4, 0x78, 0xc6, 0x29,
  0x1a, 0x1b, 0x67, 0xc0, 0x6a, 0x95, 0xc8, 0x6c, 0xf8, 0xd1, 0xab, 0xe0,
  0xd8, 0x69, 0x4e, 0x13, 0xf7, 0xe4, 0xba, 0xbb, 0xeb, 0x8a, 0xbe, 0x7f,
  0x1b, 0x99, 0xa1, 0x51, 0x2b, 0x31, 0x65, 0xec, 0x7b, 0xc7, 0xa9, 0x4a,
  0x16, 0x36, 0xde, 0xa7, 0x93, 0xf1, 0x5b, 0x6a, 0xd8, 0x11, 0x97, 0x3f,
  0x7b, 0x27, 0xdd, 0x4c, 0x47, 0xbb, 0xbd, 0xe2, 0x45, 0x23, 0xfb, 0xcd,
  0x3d, 0xaa, 0xc7, 0x0e, 0xd1, 0x87, 0xdb, 0x47, 0xd2, 0xbd, 0xfd, 0x03,
  0x05, 0x87, 0x0d, 0xfe, 0x9f, 0x1f, 0xb5, 0xb5, 0x0f, 0xb7, 0x9e, 0x06,
  0x7e, 0xf9, 0xf6, 0x92, 0xf7, 0xf5, 0xca, 0x18, 0xd1, 0x0e, 0x41, 0xcf,
  0x25, 0x4e, 0xb0, 0xc1, 0xab, 0x34, 0x02, 0x51, 0x8d, 0x03, 0x47, 0x5e,
  0xa2, 0xbe, 0xe5, 0x36, 0x78, 0x32, 0x6a, 0x9c, 0x7d, 0x4b, 0x0c, 0x9c,
  0xdb, 0xe5, 0x8d, 0x4d, 0xa2, 0x29, 0x17, 0x9b, 0xb1, 0x06, 0x1b, 0x78,
  0xf3, 0x9d, 0x6c, 0x9f, 0x3a, 0xf5, 0xb5, 0x96, 0x33, 0x99, 0x0d, 0x08,
  0x32, 0x5e, 0x25, 0x0f, 0x4e, 0x31, 0xff, 0x4a, 0xbf, 0x41, 0x23, 0xbe,
  0x00, 0x35, 0x53, 0x98, 0xcb, 0xc2, 0x04, 0x99, 0xc0, 0xe2, 0xe8, 0x85,
  0x18, 0xbe, 0x24, 0xdd, 0x43, 0xd3, 0x36, 0x9d, 0xf6, 0x66, 0xd7, 0x76,
  0xbe, 0xa7, 0xf3, 0x5b, 0xcf, 0x3d, 0x7b, 0xbd, 0x1c, 0x1d, 0xbf, 0x76,
  0xf6, 0xc1, 0xf9, 0xec, 0x49, 0xce, 0x0f, 0xe6, 0x1b, 0xd5, 0x50, 0x50,
  0x04, 0x57, 0xce, 0x0b, 0x33, 0xe7, 0xdf, 0x68, 0x29, 0x7b, 0x29, 0xd3,
  0x4e, 0xf4, 0x5d, 0xed, 0xfa, 0x8c, 0xed, 0x6f, 0x7f, 0x95, 0x32, 0x6a,
  0xe6, 0xc3, 0x77, 0x91, 0x09, 0x90, 0x8b, 0xca, 0xba, 0x47, 0xbc, 0xc4,
  0xb8, 0xad, 0xa0, 0x5c, 0x0e, 0xbf, 0x62, 0x08, 0xbe, 0x40, 0x18, 0xe2,
  0xec, 0x76, 0xc0, 0xee, 0x9b, 0xe7, 0x51, 0x97, 0xb9, 0xed, 0xe5, 0x59,
  0x0a, 0x5c, 0xba, 0x0f, 0xa6, 0xc0, 0xdb, 0x41, 0xe9, 0xcf, 0x72, 0xd7,
  0x08, 0x3b, 0xaf, 0xab, 0x03, 0xd4, 0xd4, 0xb0, 0x58, 0x5e, 0x89, 0x24,
  0x39, 0xa1, 0x1a, 0x95, 0x61, 0xb1, 0xc4, 0xcc, 0xd1, 0x0d, 0xf0, 0xb3,
  0x9a, 0xf9, 0xd6, 0x2c, 0x7c, 0xaa, 0x77, 0xbd, 0x74, 0x40, 0x3f, 0xe3,
  0x0b, 0xb5, 0xac, 0xa6, 0x2e, 0xd1, 0xf7, 0xca, 0x72, 0x46, 0x8d, 0xc4,
  0xfd, 0xf0, 0x2f, 0x5d, 0x0a, 0x67, 0x38, 0x4e, 0x14, 0x00, 0x00
};
unsigned int styles_min_css_len = 1643;
