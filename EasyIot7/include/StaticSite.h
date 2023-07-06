
#include <Arduino.h>
const uint8_t index_html[] PROGMEM = {0x1F, 0x8B, 0x08, 0x08, 0xD9, 0x11, 0xA6, 0x64, 0x04, 0x03, 0x69, 0x6E, 0x64, 0x65, 0x78, 0x2E,
                                      0x68, 0x74, 0x6D, 0x6C, 0x00, 0xC5, 0x56, 0x69, 0xAE, 0xD3, 0x30, 0x10, 0xBE, 0x8A, 0x09, 0x42,
                                      0x80, 0x84, 0x09, 0xAB, 0x54, 0x4A, 0x53, 0xD6, 0xB2, 0x48, 0x50, 0x0A, 0x94, 0xED, 0x17, 0x72,
                                      0xE2, 0x49, 0x62, 0x70, 0xEC, 0xE0, 0xA5, 0xA5, 0x5C, 0x80, 0x03, 0x20, 0x4E, 0xC0, 0x51, 0xB8,
                                      0x18, 0x63, 0x27, 0x69, 0xE9, 0x7B, 0x05, 0x21, 0xF8, 0xC1, 0x8F, 0xD7, 0x24, 0xB3, 0x7E, 0xDF,
                                      0xCC, 0xD8, 0xF3, 0x26, 0x27, 0xEE, 0x3E, 0xB9, 0xB3, 0x7C, 0xB3, 0x98, 0x91, 0xDA, 0x35, 0x72,
                                      0x3A, 0xE9, 0x7F, 0x81, 0xF1, 0x29, 0x99, 0x34, 0xE0, 0x18, 0x29, 0x6A, 0x66, 0x2C, 0xB8, 0x2C,
                                      0xF1, 0xAE, 0xA4, 0xA3, 0xA4, 0x17, 0xA3, 0xB9, 0x6B, 0x29, 0x7C, 0xF0, 0x62, 0x95, 0x25, 0xAF,
                                      0xE9, 0x8B, 0x5B, 0xF4, 0x8E, 0x6E, 0x5A, 0xE6, 0x44, 0x2E, 0x21, 0x21, 0x85, 0x56, 0x0E, 0x14,
                                      0xFA, 0x3C, 0x9C, 0x65, 0xC0, 0x2B, 0x08, 0x5E, 0x4E, 0x38, 0x09, 0xD3, 0xDB, 0x0F, 0xC8, 0x13,
                                      0xA5, 0xEF, 0x19, 0x98, 0xA4, 0x51, 0x80, 0x0A, 0x29, 0xD4, 0x7B, 0x62, 0x40, 0x66, 0x89, 0x40,
                                      0xBF, 0x84, 0xD4, 0x06, 0xCA, 0x2C, 0xE1, 0xCC, 0xB1, 0xF1, 0xB9, 0x21, 0xDD, 0x2E, 0xE2, 0x5A,
                                      0x70, 0x57, 0x67, 0x1C, 0x56, 0xA2, 0x00, 0x1A, 0x3F, 0xCE, 0x11, 0xA1, 0x84, 0x13, 0x4C, 0x52,
                                      0x5B, 0x30, 0x09, 0xD9, 0xC5, 0x73, 0xA4, 0x61, 0x1F, 0x45, 0xE3, 0x9B, 0x9D, 0xC0, 0x5B, 0x30,
                                      0xF1, 0x8B, 0xE5, 0x28, 0x50, 0x3A, 0x21, 0x8A, 0x35, 0x90, 0x25, 0x2B, 0x01, 0xEB, 0x56, 0x1B,
                                      0x97, 0xEC, 0xE1, 0xB0, 0x6E, 0x23, 0xC1, 0xD6, 0x00, 0x6E, 0x40, 0x53, 0x58, 0x9B, 0x76, 0xD2,
                                      0xF3, 0x8D, 0x50, 0xE7, 0xF1, 0xF3, 0xC6, 0xE8, 0xFC, 0xE5, 0x0B, 0x17, 0x93, 0xE9, 0x24, 0x8D,
                                      0xC5, 0x9A, 0xE4, 0x9A, 0x6F, 0x48, 0x21, 0x99, 0xB5, 0x59, 0x42, 0xEC, 0x7B, 0xA1, 0x68, 0x2E,
                                      0x3D, 0x10, 0x2B, 0x38, 0xE4, 0xCC, 0x50, 0xF4, 0x12, 0x68, 0xCC, 0xC5, 0x0A, 0x33, 0x05, 0x0F,
                                      0x30, 0x83, 0x75, 0xC3, 0xD0, 0xB8, 0x13, 0x05, 0x18, 0x8A, 0xAD, 0x06, 0x0D, 0xBE, 0xA2, 0x2F,
                                      0x89, 0x0F, 0x84, 0xEF, 0xB0, 0xBC, 0x05, 0x75, 0xBA, 0x4D, 0x88, 0xD1, 0x12, 0xA2, 0x5E, 0x54,
                                      0x28, 0xD4, 0x2A, 0x38, 0x7A, 0x39, 0xF8, 0x6D, 0x93, 0x82, 0xF2, 0x09, 0x09, 0x95, 0x0C, 0x95,
                                      0xAA, 0x42, 0x13, 0x9D, 0x01, 0xE8, 0xC8, 0x0E, 0xC6, 0x88, 0x8A, 0xF5, 0x2C, 0x4F, 0x26, 0x83,
                                      0x30, 0x78, 0x52, 0xE1, 0xA0, 0xE9, 0xDD, 0xC3, 0x37, 0xE6, 0xD3, 0x3C, 0x3A, 0xDB, 0x96, 0xA9,
                                      0xC1, 0x52, 0x32, 0x55, 0xD1, 0x4E, 0x31, 0xFF, 0xFE, 0x65, 0x92, 0x06, 0x1D, 0x16, 0x85, 0xE1,
                                      0x9F, 0x14, 0x53, 0xCC, 0xF3, 0xE7, 0xE1, 0xBB, 0xA6, 0xDA, 0x83, 0x19, 0x4A, 0x60, 0xCE, 0x9B,
                                      0xA0, 0xBC, 0xF7, 0x62, 0xFE, 0xFD, 0xF3, 0xF7, 0xAF, 0xB3, 0xE7, 0x3F, 0xE5, 0x22, 0x31, 0xD9,
                                      0xDF, 0x91, 0x5A, 0x8B, 0x52, 0x0C, 0x29, 0xA7, 0xAF, 0x1E, 0xD2, 0x7B, 0xE2, 0xA7, 0xC0, 0xFF,
                                      0x10, 0x57, 0xE0, 0xC0, 0x56, 0x26, 0x76, 0xC7, 0x6E, 0xE3, 0x1F, 0x27, 0xB6, 0x6F, 0xF6, 0x70,
                                      0xBE, 0x9C, 0xDD, 0x7F, 0x76, 0x6B, 0x8F, 0xE0, 0x31, 0x38, 0xA9, 0x97, 0x81, 0x31, 0x76, 0x3F,
                                      0x3C, 0xBA, 0xC9, 0xC1, 0x37, 0x9C, 0xAD, 0x21, 0x72, 0x1C, 0x77, 0x74, 0x02, 0x99, 0xEC, 0x2B,
                                      0x5A, 0x2F, 0x25, 0x95, 0x50, 0xE2, 0x60, 0xC7, 0x71, 0xC6, 0xC9, 0xD6, 0x52, 0x9B, 0x31, 0x59,
                                      0xD7, 0x48, 0xE0, 0x3A, 0xA9, 0x41, 0x54, 0xB5, 0x1B, 0x93, 0x8B, 0x17, 0xDA, 0x8F, 0xD7, 0x7B,
                                      0xDF, 0xC1, 0xB2, 0x65, 0x9C, 0x0B, 0x55, 0x8D, 0xC9, 0xD5, 0xA0, 0xEB, 0xA8, 0xEC, 0x47, 0x39,
                                      0x39, 0x1A, 0xE5, 0xE5, 0xB5, 0x02, 0x95, 0xAF, 0x04, 0xD6, 0x71, 0x4C, 0x3A, 0xE8, 0x43, 0x3F,
                                      0x05, 0xC7, 0xD1, 0xC4, 0xD9, 0x7C, 0x2B, 0x73, 0xC4, 0x75, 0x54, 0xD7, 0x75, 0x82, 0x5A, 0x51,
                                      0x29, 0x86, 0xEA, 0x0B, 0xA7, 0xB6, 0xBC, 0x11, 0x43, 0xFF, 0xFB, 0x3F, 0xA8, 0x3C, 0x7E, 0xBA,
                                      0x5C, 0x1E, 0x62, 0xD2, 0x7C, 0x70, 0x6E, 0xC7, 0xE4, 0xB8, 0x2E, 0x9E, 0x57, 0x3C, 0x15, 0x1C,
                                      0xAC, 0xC4, 0x53, 0xCA, 0xF5, 0x9F, 0xF0, 0x31, 0x88, 0x79, 0x47, 0xA8, 0xC4, 0x4B, 0x0F, 0x0B,
                                      0xF2, 0x09, 0x90, 0xC5, 0x15, 0x44, 0x4A, 0x7E, 0x66, 0x18, 0x39, 0xE5, 0xDE, 0x39, 0xBD, 0x85,
                                      0x9E, 0x6B, 0xC3, 0xB1, 0xF1, 0xB9, 0x46, 0x61, 0x13, 0x6B, 0x43, 0x0D, 0xE3, 0xC2, 0xDB, 0xBE,
                                      0x0A, 0x24, 0x67, 0xC5, 0xFB, 0xCA, 0x68, 0xAF, 0x38, 0xED, 0x43, 0x0D, 0x3C, 0x49, 0xF4, 0x45,
                                      0x81, 0xD2, 0x0A, 0x43, 0x13, 0xAD, 0x0A, 0x29, 0x8A, 0xF7, 0x38, 0xA4, 0x9A, 0x71, 0xFB, 0x08,
                                      0x27, 0xD5, 0xB3, 0x0A, 0xCE, 0x9C, 0x5E, 0x2C, 0x4F, 0x9F, 0xC5, 0xCC, 0x8B, 0x25, 0x56, 0xA4,
                                      0x4B, 0xFE, 0x7B, 0x14, 0x91, 0xD1, 0x11, 0x18, 0xFF, 0x8E, 0x62, 0x36, 0x8F, 0x28, 0x66, 0xF3,
                                      0x9F, 0x50, 0xFC, 0xAA, 0xAE, 0xFD, 0xE6, 0xA0, 0x6B, 0xC3, 0xDA, 0xB6, 0xBB, 0x62, 0x2D, 0x14,
                                      0xE1, 0xBC, 0x1D, 0xB1, 0x88, 0x3B, 0x06, 0xEF, 0x62, 0xC4, 0x5E, 0x4A, 0x2F, 0x38, 0x1E, 0x66,
                                      0x5B, 0xE7, 0x9A, 0x19, 0x4E, 0xB7, 0x9A, 0xD0, 0xEC, 0xCE, 0x7B, 0x97, 0xB1, 0xD4, 0xDA, 0x1D,
                                      0xB9, 0xCD, 0x83, 0x28, 0xA6, 0x3A, 0xDC, 0x60, 0x52, 0x0B, 0xCE, 0x41, 0xD1, 0x8F, 0x36, 0x39,
                                      0x70, 0x23, 0xAC, 0xC0, 0xD8, 0x78, 0xA5, 0xBF, 0xC4, 0x97, 0xEF, 0xDF, 0xF4, 0x81, 0xD9, 0xEB,
                                      0x4D, 0xBA, 0xF1, 0xA3, 0x51, 0xBF, 0xC3, 0x63, 0x9D, 0xD1, 0xAA, 0xEA, 0x36, 0xED, 0x93, 0xD2,
                                      0x00, 0xA1, 0x64, 0xA1, 0xD7, 0x60, 0x80, 0x93, 0x7C, 0x43, 0xB6, 0xF7, 0x57, 0x58, 0xE0, 0x76,
                                      0x9C, 0xA6, 0x95, 0x70, 0xB5, 0xCF, 0xCF, 0x17, 0xBA, 0x49, 0x73, 0xE3, 0x95, 0xAE, 0x71, 0x1D,
                                      0xB2, 0xD1, 0xA5, 0xF4, 0xF6, 0x03, 0x3A, 0x63, 0x76, 0x43, 0x1F, 0x6A, 0xDC, 0x8E, 0xB7, 0x83,
                                      0x86, 0x3C, 0x08, 0xAA, 0x78, 0xDF, 0xDE, 0x24, 0x97, 0x2E, 0x5C, 0x1C, 0xE1, 0xE2, 0x25, 0x0B,
                                      0x94, 0xF9, 0x8A, 0xC9, 0x49, 0xDA, 0xE7, 0x9D, 0xA4, 0x1D, 0xFB, 0xE9, 0x00, 0xA7, 0x30, 0xA2,
                                      0x75, 0xC4, 0x9A, 0x22, 0x4B, 0xDE, 0xD9, 0xF4, 0xDD, 0x07, 0x0F, 0x66, 0x13, 0xB7, 0xE8, 0xBB,
                                      0x40, 0x3F, 0xED, 0xF4, 0xC7, 0xEC, 0x84, 0xE2, 0xF0, 0x11, 0x4D, 0x86, 0x3D, 0xBB, 0x33, 0x4C,
                                      0xC3, 0xAA, 0xC5, 0x47, 0xFC, 0x57, 0xE5, 0x07, 0xCB, 0xA5, 0x06, 0x46, 0xC0, 0x08, 0x00, 0x00};

const uint8_t integrations_html[] PROGMEM = {0x1F, 0x8B, 0x08, 0x08, 0x04, 0x09, 0xA6, 0x64, 0x04, 0x03, 0x69, 0x6E, 0x74, 0x65, 0x67, 0x72,
                                             0x61, 0x74, 0x69, 0x6F, 0x6E, 0x73, 0x2E, 0x68, 0x74, 0x6D, 0x6C, 0x00, 0xBD, 0x94, 0xCD, 0x72,
                                             0xDA, 0x30, 0x10, 0xC7, 0x5F, 0x45, 0xA3, 0x53, 0x7B, 0x30, 0xC6, 0x90, 0x10, 0xC8, 0x18, 0x66,
                                             0xD2, 0x24, 0xD3, 0xC9, 0x81, 0x94, 0x0E, 0xC9, 0xB9, 0x23, 0xAC, 0x35, 0xD6, 0x20, 0x5B, 0xAE,
                                             0x24, 0x03, 0xE9, 0x0B, 0xF5, 0x41, 0xFA, 0x62, 0x5D, 0x63, 0xC0, 0x60, 0xC3, 0x34, 0xB4, 0xD0,
                                             0x83, 0x8D, 0xD9, 0x5D, 0xEF, 0xC7, 0xCF, 0xDA, 0xBF, 0xCF, 0xC5, 0x9C, 0x04, 0x92, 0x19, 0xD3,
                                             0xA7, 0x13, 0xB5, 0x24, 0x78, 0x39, 0xA9, 0x16, 0x31, 0xD3, 0x6F, 0x74, 0x40, 0x2A, 0x5E, 0x27,
                                             0x02, 0xC6, 0x41, 0x93, 0x85, 0xB0, 0x91, 0x33, 0x51, 0x1A, 0x9F, 0xE9, 0xC0, 0x8F, 0xDA, 0xBB,
                                             0x21, 0x56, 0x58, 0x09, 0x74, 0x30, 0xFC, 0xFA, 0xF2, 0x42, 0x7C, 0x37, 0x6A, 0x0F, 0x7C, 0x17,
                                             0x93, 0x0C, 0xFC, 0x50, 0xE9, 0x18, 0x13, 0x86, 0x02, 0x24, 0x37, 0x60, 0x89, 0xE0, 0x7D, 0x1A,
                                             0x86, 0x94, 0x70, 0x61, 0xD8, 0x44, 0x02, 0xAF, 0x15, 0xC3, 0x02, 0xBC, 0xDA, 0x83, 0x56, 0x8B,
                                             0x8A, 0x25, 0x50, 0xD2, 0x89, 0xB9, 0x73, 0x45, 0xF2, 0x87, 0xA5, 0x71, 0xBC, 0x56, 0x25, 0x20,
                                             0xAF, 0xEB, 0x4C, 0xB5, 0xCA, 0x52, 0x6C, 0x55, 0xB2, 0x09, 0x48, 0x82, 0xA6, 0x3E, 0x8D, 0xBF,
                                             0x5B, 0xFB, 0x4D, 0xA0, 0xF1, 0x93, 0x56, 0x33, 0x9C, 0xE9, 0x69, 0x44, 0x5C, 0xF2, 0xF0, 0x3C,
                                             0xF6, 0xDD, 0x55, 0x10, 0x26, 0x11, 0x49, 0x9A, 0xD9, 0xBD, 0x34, 0x81, 0x4A, 0xAC, 0x56, 0x92,
                                             0x92, 0x84, 0xC5, 0x50, 0xA4, 0x78, 0x4A, 0x1F, 0x12, 0x43, 0x89, 0x7D, 0x4B, 0xD1, 0x60, 0x61,
                                             0x69, 0x29, 0x89, 0xD9, 0x52, 0x42, 0x32, 0xB5, 0x51, 0x9F, 0x5E, 0x35, 0x29, 0x0E, 0x5A, 0x16,
                                             0x23, 0xA9, 0x64, 0x01, 0x44, 0x4A, 0x72, 0xC0, 0x16, 0x60, 0x79, 0x4B, 0xBC, 0x5E, 0xAB, 0xE1,
                                             0x75, 0xBA, 0x0D, 0xAF, 0xD1, 0x6E, 0x52, 0x17, 0xAB, 0xAE, 0x68, 0x15, 0xF7, 0xF3, 0xCD, 0x99,
                                             0x19, 0xD0, 0x79, 0xCF, 0xE8, 0x32, 0x29, 0x4B, 0x36, 0xEF, 0x48, 0x96, 0x4C, 0x9D, 0xD2, 0xF7,
                                             0x6A, 0x85, 0x14, 0x3F, 0x18, 0x57, 0xDA, 0x77, 0xF3, 0xB0, 0xC1, 0xBB, 0x58, 0xB0, 0xCC, 0xAA,
                                             0x40, 0xC5, 0xA9, 0x04, 0x8B, 0x08, 0x54, 0x18, 0xEE, 0xE2, 0x79, 0x5D, 0x27, 0x3F, 0x4A, 0xA8,
                                             0x5D, 0x12, 0x2A, 0xDB, 0x3C, 0xC0, 0x49, 0xB0, 0x18, 0xF1, 0x5C, 0x86, 0x4E, 0x8A, 0x21, 0x0B,
                                             0x3C, 0xCD, 0x87, 0xE8, 0x94, 0xBE, 0x11, 0x93, 0x6C, 0xAE, 0x19, 0x19, 0xA1, 0x05, 0x4E, 0x01,
                                             0x54, 0xD2, 0x18, 0xAD, 0x93, 0xED, 0x11, 0xE8, 0xB4, 0x36, 0x70, 0xB6, 0xB5, 0xEA, 0x4C, 0x77,
                                             0x20, 0x6D, 0xA3, 0xEA, 0x90, 0x4C, 0x96, 0x82, 0x36, 0x58, 0x35, 0x99, 0x6E, 0xA2, 0xCA, 0x33,
                                             0x55, 0xBF, 0x57, 0x17, 0x2E, 0x54, 0xCA, 0xE2, 0x26, 0x18, 0x36, 0x87, 0x1C, 0xDF, 0x24, 0xB3,
                                             0x56, 0x25, 0xEB, 0xDE, 0x8A, 0x3F, 0x94, 0xA8, 0x24, 0x90, 0x22, 0x98, 0xF5, 0x69, 0x1E, 0x35,
                                             0xC4, 0x7E, 0x3E, 0x7C, 0xA4, 0xDB, 0x1C, 0x36, 0x21, 0x78, 0x95, 0xFA, 0x51, 0xC7, 0x59, 0xE4,
                                             0xFE, 0x9C, 0x31, 0xCD, 0x59, 0x79, 0xCA, 0x8A, 0xE4, 0x65, 0xA3, 0x1B, 0x91, 0xC8, 0x2D, 0x2B,
                                             0xDD, 0x28, 0x1C, 0x97, 0xD2, 0xAA, 0xC7, 0xE1, 0x97, 0xE7, 0xFB, 0xE1, 0xB8, 0x26, 0x57, 0xFF,
                                             0x4F, 0x92, 0x0E, 0x90, 0x62, 0x9C, 0x6B, 0x30, 0x06, 0xBB, 0x4B, 0xB0, 0x75, 0xF8, 0xF5, 0x53,
                                             0x9D, 0x7E, 0xE6, 0x20, 0xC6, 0xCF, 0x15, 0x9B, 0x31, 0xE8, 0x39, 0xE8, 0xA3, 0x2B, 0xD8, 0x5D,
                                             0xAD, 0x60, 0x35, 0xBA, 0x7E, 0xBA, 0x22, 0x6B, 0xD3, 0x5B, 0xD7, 0x5D, 0x87, 0x35, 0x94, 0x9E,
                                             0x9E, 0x77, 0x23, 0x0F, 0x6D, 0x9F, 0x86, 0x50, 0x2C, 0x71, 0xF7, 0x56, 0xBF, 0x7F, 0x4F, 0x60,
                                             0xC4, 0x6C, 0xF4, 0x47, 0x91, 0xDE, 0x8D, 0xAD, 0x4F, 0xBF, 0x99, 0xFB, 0xBC, 0x33, 0xE7, 0x2A,
                                             0xB4, 0xAD, 0x7C, 0x97, 0x8A, 0x19, 0x1C, 0x5C, 0x1B, 0xB6, 0xF6, 0xDC, 0x47, 0xB8, 0x3F, 0x84,
                                             0x33, 0x72, 0x37, 0x7A, 0x3A, 0x05, 0xC6, 0x11, 0x8D, 0xAE, 0x54, 0x7E, 0x2F, 0xA1, 0x75, 0x74,
                                             0x9D, 0x51, 0x78, 0xD5, 0xB9, 0xF6, 0x02, 0xE8, 0x75, 0x78, 0xAB, 0xD9, 0xEB, 0xF2, 0xEB, 0x9B,
                                             0xF6, 0xCD, 0x35, 0xEF, 0x75, 0x9B, 0x41, 0xC7, 0x6B, 0x81, 0xD7, 0xF5, 0x2E, 0x2A, 0x46, 0x8F,
                                             0x45, 0x73, 0xFF, 0xAE, 0x47, 0xA4, 0x2A, 0x48, 0x7B, 0x22, 0xF4, 0x1B, 0xC4, 0x67, 0xF3, 0xDD,
                                             0x31, 0x09, 0x00, 0x00};

const uint8_t wifi_html[] PROGMEM = {
    0x1F,
    0x8B,
    0x08,
    0x08,
    0xCB,
    0x5B,
    0x55,
    0x5F,
    0x04,
    0x03,
    0x77,
    0x69,
    0x66,
    0x69,
    0x2E,
    0x68,
    0x74,
    0x6D,
    0x6C,
    0x00,
    0xC5,
    0x56,
    0xCB,
    0x8E,
    0x9B,
    0x30,
    0x14,
    0xFD,
    0x15,
    0xCB,
    0xAB,
    0x76,
    0x41,
    0x5E,
    0x93,
    0x44,
    0xED,
    0x88,
    0x20,
    0x55,
    0x1D,
    0x35,
    0xCD,
    0x62,
    0xA4,
    0xA8,
    0x59,
    0xCC,
    0x72,
    0x64,
    0xF0,
    0x4D,
    0xB0,
    0x62,
    0xB0,
    0x65,
    0x9B,
    0x84,
    0xEC,
    0xFA,
    0x1B,
    0x5D,
    0xF6,
    0x5B,
    0xFA,
    0x27,
    0xFD,
    0x92,
    0xDA,
    0x04,
    0x42,
    0x08,
    0xE9,
    0x88,
    0xB4,
    0x55,
    0x67,
    0x01,
    0xF8,
    0xC5,
    0xB9,
    0xF7,
    0x9C,
    0x73,
    0xAF,
    0xC0,
    0xA7,
    0x6C,
    0x87,
    0x22,
    0x4E,
    0xB4,
    0x9E,
    0xE1,
    0x50,
    0xE4,
    0xC8,
    0x5E,
    0x9E,
    0x54,
    0x2C,
    0x21,
    0xEA,
    0x80,
    0x03,
    0xE4,
    0xAF,
    0x85,
    0x4A,
    0xDC,
    0x83,
    0x01,
    0xA7,
    0x1A,
    0x0C,
    0x62,
    0x74,
    0x86,
    0xD7,
    0x6B,
    0x8C,
    0x28,
    0xD3,
    0x24,
    0xE4,
    0x40,
    0xED,
    0x5E,
    0x13,
    0xC0,
    0x8B,
    0x81,
    0x50,
    0x50,
    0x68,
    0xCF,
    0x4C,
    0xEC,
    0x85,
    0x42,
    0xD9,
    0x31,
    0x0E,
    0xFC,
    0xF8,
    0xEE,
    0xFC,
    0x88,
    0x61,
    0x86,
    0x03,
    0x0E,
    0x3E,
    0x2C,
    0xD1,
    0xD3,
    0xC2,
    0xFB,
    0xB4,
    0x40,
    0x7E,
    0x3F,
    0xBE,
    0x0B,
    0xFC,
    0xBE,
    0x05,
    0x0A,
    0x2E,
    0xD0,
    0x2C,
    0x02,
    0x75,
    0x79,
    0x9C,
    0x2F,
    0x2B,
    0xB1,
    0xBF,
    0x58,
    0x89,
    0x04,
    0xF7,
    0x12,
    0xEA,
    0x8D,
    0x91,
    0x1B,
    0xE4,
    0xDA,
    0x1B,
    0x8E,
    0x2E,
    0x0E,
    0x38,
    0x16,
    0xDE,
    0x46,
    0x89,
    0x4C,
    0xDA,
    0x5C,
    0x38,
    0x09,
    0x81,
    0x23,
    0xBB,
    0x34,
    0xC3,
    0x44,
    0xAE,
    0x20,
    0x52,
    0x60,
    0x70,
    0xB0,
    0xB4,
    0xE7,
    0xF6,
    0x36,
    0x5B,
    0xBF,
    0x5F,
    0x6C,
    0xDB,
    0xD7,
    0x59,
    0x2A,
    0x33,
    0x83,
    0x48,
    0x66,
    0x44,
    0x24,
    0x12,
    0xC9,
    0xC1,
    0xC0,
    0x0C,
    0x0B,
    0x4B,
    0xBD,
    0x81,
    0x19,
    0x89,
    0xD4,
    0x28,
    0xC1,
    0x31,
    0x32,
    0x07,
    0x69,
    0xF7,
    0x65,
    0x89,
    0x82,
    0x51,
    0x4A,
    0x12,
    0x38,
    0xC3,
    0x2F,
    0x74,
    0xAB,
    0x67,
    0x92,
    0x93,
    0x08,
    0x62,
    0xC1,
    0x29,
    0xD8,
    0x24,
    0x20,
    0xBF,
    0x47,
    0x3A,
    0x93,
    0xA0,
    0x96,
    0xD5,
    0xDB,
    0x7D,
    0x1B,
    0xBE,
    0x50,
    0xA3,
    0x7D,
    0xFF,
    0x73,
    0xB5,
    0xAF,
    0x4A,
    0x5D,
    0xE4,
    0xB5,
    0x67,
    0x6B,
    0xF6,
    0xCC,
    0xC5,
    0xE6,
    0xD9,
    0x9E,
    0x3E,
    0xD1,
    0x8B,
    0x19,
    0xA5,
    0x90,
    0xA2,
    0xDF,
    0xC7,
    0x98,
    0x5E,
    0x89,
    0xE1,
    0x6B,
    0x49,
    0xD2,
    0x26,
    0x26,
    0x0F,
    0xB9,
    0x5D,
    0xEF,
    0xBB,
    0x0D,
    0xFB,
    0x88,
    0xA7,
    0xAF,
    0xE2,
    0xB3,
    0xD6,
    0x8C,
    0xE2,
    0x60,
    0xB5,
    0x5A,
    0x3C,
    0x5C,
    0xF8,
    0x7B,
    0xDD,
    0xCC,
    0x84,
    0xE4,
    0x1C,
    0xD2,
    0x8D,
    0x89,
    0x67,
    0x78,
    0x74,
    0x87,
    0xDB,
    0x35,
    0x50,
    0xD9,
    0x6D,
    0x20,
    0x37,
    0x47,
    0xAB,
    0x8F,
    0x84,
    0x5D,
    0x00,
    0x67,
    0x75,
    0x19,
    0xB0,
    0x6D,
    0x73,
    0x02,
    0x99,
    0x3B,
    0x87,
    0xFB,
    0x0D,
    0x4F,
    0xFF,
    0x09,
    0xC5,
    0x42,
    0x70,
    0x5D,
    0xD4,
    0x57,
    0xE9,
    0x43,
    0xF5,
    0x06,
    0x27,
    0xE9,
    0xC6,
    0xAB,
    0x2A,
    0xD3,
    0x55,
    0x3A,
    0x27,
    0x3B,
    0x45,
    0x90,
    0xAB,
    0x36,
    0x28,
    0x8D,
    0xE9,
    0xA4,
    0xCA,
    0x19,
    0xCF,
    0x22,
    0x4C,
    0x43,
    0xA7,
    0xE9,
    0xE0,
    0x05,
    0x9D,
    0x4E,
    0xC1,
    0xEB,
    0xD2,
    0xD0,
    0x2F,
    0x76,
    0x82,
    0x3C,
    0xEB,
    0x84,
    0x5B,
    0x94,
    0x6A,
    0x78,
    0x6E,
    0x88,
    0x61,
    0xD1,
    0xC2,
    0xCA,
    0xF4,
    0xF0,
    0xF9,
    0xE3,
    0xB2,
    0x13,
    0x43,
    0x6D,
    0x0E,
    0xBC,
    0xA0,
    0x48,
    0x4D,
    0x7C,
    0x8F,
    0xC6,
    0x03,
    0x99,
    0x57,
    0x0C,
    0xA2,
    0x18,
    0xA2,
    0xAD,
    0xAD,
    0x53,
    0xC7,
    0xA0,
    0x86,
    0xAE,
    0x34,
    0xA9,
    0xE7,
    0x3B,
    0xC2,
    0x33,
    0x57,
    0x18,
    0x2A,
    0x73,
    0xDD,
    0x10,
    0xAA,
    0x0E,
    0x1D,
    0xFC,
    0x3F,
    0x2A,
    0x7F,
    0xD1,
    0x8D,
    0xFF,
    0xF5,
    0xAA,
    0x5E,
    0x48,
    0xC7,
    0xBA,
    0x1A,
    0x37,
    0x5C,
    0x1F,
    0x4E,
    0xAE,
    0x38,
    0x38,
    0x7C,
    0x3F,
    0xEA,
    0x0D,
    0xA7,
    0xEF,
    0x7A,
    0xC3,
    0xDE,
    0x68,
    0xD0,
    0xCD,
    0xC0,
    0x9F,
    0x5F,
    0xBF,
    0xDD,
    0x48,
    0xAB,
    0x51,
    0xED,
    0x73,
    0x62,
    0x60,
    0x4F,
    0x0E,
    0x9D,
    0x28,
    0xD6,
    0xB4,
    0xE6,
    0x7B,
    0x47,
    0xAB,
    0x1A,
    0xDF,
    0x44,
    0x6B,
    0x32,
    0xEE,
    0xC6,
    0xEB,
    0x6F,
    0x48,
    0xB5,
    0x5B,
    0x38,
    0x05,
    0x93,
    0x10,
    0xBD,
    0xC5,
    0xC1,
    0xE3,
    0x8F,
    0xEF,
    0x3A,
    0x22,
    0xB6,
    0x85,
    0x29,
    0xA0,
    0x2F,
    0x40,
    0x6F,
    0x6A,
    0xE2,
    0x0B,
    0x96,
    0xB5,
    0x1C,
    0x8F,
    0x16,
    0xBA,
    0x10,
    0xA4,
    0x9E,
    0xB5,
    0x35,
    0x18,
    0x4D,
    0x26,
    0xBD,
    0xEA,
    0x1A,
    0xDC,
    0xF2,
    0x99,
    0x5A,
    0x0B,
    0x61,
    0xEC,
    0x27,
    0x44,
    0x93,
    0x1D,
    0x38,
    0x31,
    0xC2,
    0xCC,
    0x18,
    0x91,
    0x96,
    0xF5,
    0x76,
    0x9C,
    0x60,
    0x24,
    0xD2,
    0x88,
    0xB3,
    0x68,
    0x3B,
    0xC3,
    0xEE,
    0xD4,
    0x93,
    0x4D,
    0xE2,
    0xCD,
    0x5B,
    0x7C,
    0xC2,
    0x30,
    0x29,
    0xB2,
    0x57,
    0xFD,
    0x67,
    0xD2,
    0x96,
    0xE7,
    0x88,
    0x3D,
    0xCF,
    0x88,
    0xA2,
    0x44,
    0x9D,
    0x24,
    0x39,
    0x82,
    0xD7,
    0x89,
    0x56,
    0x3F,
    0x32,
    0x6E,
    0xC5,
    0x29,
    0x53,
    0xE6,
    0xFB,
    0x0B,
    0x37,
    0xE8,
    0x8C,
    0xA9,
    0x06,
    0x09,
    0x00,
    0x00,

};

const uint8_t devices_html[] PROGMEM = {
    0x1F,
    0x8B,
    0x08,
    0x08,
    0x53,
    0xF0,
    0x6C,
    0x5D,
    0x04,
    0x03,
    0x64,
    0x65,
    0x76,
    0x69,
    0x63,
    0x65,
    0x73,
    0x2E,
    0x68,
    0x74,
    0x6D,
    0x6C,
    0x00,
    0xDD,
    0x52,
    0x3D,
    0x6F,
    0xDC,
    0x30,
    0x0C,
    0xFD,
    0x2B,
    0x84,
    0x8A,
    0x02,
    0x2D,
    0x50,
    0xE3,
    0xEC,
    0x38,
    0x19,
    0xCE,
    0x3E,
    0xDF,
    0xD0,
    0x29,
    0x1D,
    0x3A,
    0xA5,
    0xB8,
    0xA1,
    0x4B,
    0x20,
    0xDB,
    0xB4,
    0x4D,
    0x44,
    0x91,
    0x0C,
    0x89,
    0xF7,
    0xF5,
    0xEF,
    0x43,
    0xC9,
    0xB8,
    0xE0,
    0x10,
    0x64,
    0xC8,
    0x90,
    0x29,
    0x86,
    0x25,
    0x3F,
    0x10,
    0xE4,
    0x23,
    0xDF,
    0xA3,
    0x37,
    0x3D,
    0x1D,
    0xA0,
    0x33,
    0x3A,
    0x84,
    0x46,
    0xB5,
    0xEE,
    0x04,
    0x72,
    0xB2,
    0xE0,
    0x0C,
    0xF5,
    0xD0,
    0xE3,
    0x81,
    0x3A,
    0xCC,
    0x62,
    0xA0,
    0x1D,
    0xD5,
    0x16,
    0x52,
    0x66,
    0xE0,
    0xB3,
    0xC1,
    0x46,
    0x0D,
    0xC6,
    0x69,
    0xAE,
    0xC0,
    0xE0,
    0xC0,
    0x35,
    0x1C,
    0xA9,
    0xE7,
    0xA9,
    0x82,
    0x22,
    0xCF,
    0xBF,
    0xD7,
    0xC0,
    0x78,
    0xE2,
    0x4C,
    0x1B,
    0x1A,
    0x6D,
    0x05,
    0x1D,
    0x5A,
    0x46,
    0x5F,
    0x5F,
    0x6A,
    0xA9,
    0x6F,
    0x94,
    0x47,
    0xDD,
    0x93,
    0x1D,
    0x83,
    0xBA,
    0x30,
    0x75,
    0xCE,
    0x38,
    0x5F,
    0xC1,
    0x71,
    0x22,
    0xC6,
    0x1A,
    0x9E,
    0xB5,
    0x1F,
    0x49,
    0x2A,
    0xF5,
    0x9E,
    0x5D,
    0xFD,
    0x0E,
    0x15,
    0x4C,
    0x48,
    0xE3,
    0x24,
    0x9D,
    0xCB,
    0x7C,
    0x3E,
    0xD5,
    0xD0,
    0x53,
    0x98,
    0x8D,
    0x3E,
    0x57,
    0x40,
    0xD6,
    0x90,
    0xC5,
    0x6C,
    0x30,
    0x28,
    0xD1,
    0xD8,
    0x6F,
    0x25,
    0x0D,
    0xB7,
    0xCB,
    0xFD,
    0x46,
    0x61,
    0x36,
    0xC9,
    0x08,
    0xE8,
    0x63,
    0xD2,
    0x54,
    0x5E,
    0xC7,
    0x99,
    0xD8,
    0xA0,
    0xDA,
    0x6E,
    0xC2,
    0xAC,
    0xED,
    0x25,
    0x6E,
    0xB4,
    0x1D,
    0xB3,
    0x70,
    0x24,
    0xEE,
    0x26,
    0x0C,
    0x6A,
    0xFB,
    0x27,
    0x0E,
    0xE1,
    0xF7,
    0x33,
    0x3B,
    0x8F,
    0x61,
    0xB3,
    0x8A,
    0x99,
    0x42,
    0xD3,
    0xEE,
    0x99,
    0xDD,
    0x6B,
    0x49,
    0xCB,
    0x16,
    0xE4,
    0x64,
    0xB3,
    0x27,
    0x51,
    0x73,
    0x4E,
    0xB8,
    0x73,
    0x96,
    0xBD,
    0x33,
    0x0A,
    0x9C,
    0xED,
    0x0C,
    0x75,
    0x4F,
    0x92,
    0xB5,
    0x27,
    0xD3,
    0x3F,
    0x24,
    0xE2,
    0x7F,
    0xF8,
    0x2C,
    0x22,
    0x18,
    0x7F,
    0xFC,
    0x8C,
    0x33,
    0x85,
    0xC3,
    0x78,
    0xB1,
    0x66,
    0xF1,
    0xF5,
    0xE6,
    0x56,
    0x94,
    0x2E,
    0xB2,
    0x13,
    0x56,
    0x70,
    0x20,
    0x3C,
    0xFE,
    0x76,
    0xA7,
    0x46,
    0xE5,
    0x90,
    0xC3,
    0xCD,
    0xAD,
    0xBC,
    0xB1,
    0x70,
    0xD6,
    0x3C,
    0xC1,
    0x40,
    0xC6,
    0x34,
    0xEA,
    0x5B,
    0x9E,
    0x1E,
    0x05,
    0x62,
    0xF8,
    0xDF,
    0x62,
    0xFD,
    0xAB,
    0x28,
    0xEF,
    0x8B,
    0x72,
    0x57,
    0xAC,
    0xEF,
    0x8B,
    0x62,
    0x27,
    0xF8,
    0x6E,
    0x57,
    0x14,
    0x11,
    0xDE,
    0x49,
    0x34,
    0xC1,
    0xB5,
    0x44,
    0xFF,
    0xAB,
    0x55,
    0xB4,
    0x4D,
    0xDA,
    0xC7,
    0xCF,
    0x22,
    0x29,
    0xA2,
    0xA9,
    0x94,
    0xFB,
    0x5D,
    0x1F,
    0x5B,
    0xD7,
    0x9F,
    0xAF,
    0x57,
    0xBB,
    0xD8,
    0xF4,
    0x28,
    0x5A,
    0x07,
    0x1A,
    0xD5,
    0x75,
    0x66,
    0xFA,
    0x93,
    0xA2,
    0x7F,
    0x9F,
    0xBB,
    0x16,
    0xB4,
    0xC1,
    0x79,
    0x61,
    0x7D,
    0x10,
    0xF0,
    0x49,
    0x0B,
    0x49,
    0x4C,
    0x5F,
    0x66,
    0x21,
    0x8B,
    0x41,
    0x1F,
    0xDF,
    0x48,
    0xBA,
    0x5F,
    0x00,
    0xB8,
    0xC7,
    0xB5,
    0x60,
    0x11,
    0x04,
    0x00,
    0x00,
};

const uint8_t node_html[] PROGMEM = {
    0x1F,
    0x8B,
    0x08,
    0x08,
    0xD3,
    0xE2,
    0x5C,
    0x5F,
    0x04,
    0x03,
    0x6E,
    0x6F,
    0x64,
    0x65,
    0x2E,
    0x68,
    0x74,
    0x6D,
    0x6C,
    0x00,
    0xB5,
    0x56,
    0x4B,
    0x6E,
    0xDB,
    0x30,
    0x10,
    0xBD,
    0x0A,
    0xC1,
    0x6E,
    0xDA,
    0x05,
    0xF3,
    0x05,
    0x82,
    0x36,
    0xB1,
    0x0D,
    0x04,
    0x4E,
    0x53,
    0x18,
    0x68,
    0xD3,
    0x22,
    0x49,
    0x37,
    0xDD,
    0x8D,
    0xC4,
    0x91,
    0x45,
    0x84,
    0x22,
    0x55,
    0x8A,
    0xF2,
    0xE7,
    0x1C,
    0x3D,
    0x41,
    0xD1,
    0x45,
    0x0F,
    0xE2,
    0x8B,
    0x75,
    0x28,
    0x4B,
    0xB2,
    0x23,
    0xCB,
    0x68,
    0x82,
    0x36,
    0x0B,
    0xC9,
    0x14,
    0x87,
    0x9A,
    0x79,
    0x6F,
    0xE6,
    0x8D,
    0xC6,
    0x03,
    0xA9,
    0x66,
    0x2C,
    0xD6,
    0x50,
    0x14,
    0x43,
    0x1E,
    0xD9,
    0x05,
    0xA3,
    0x4B,
    0xE4,
    0x4E,
    0x65,
    0xE0,
    0x96,
    0x7C,
    0xC4,
    0x3A,
    0x56,
    0x91,
    0x22,
    0x48,
    0x74,
    0x6C,
    0xAE,
    0x7C,
    0x2A,
    0x22,
    0xEB,
    0x68,
    0xCD,
    0x47,
    0x83,
    0xF4,
    0x74,
    0xFB,
    0x88,
    0x57,
    0x5E,
    0x23,
    0xA3,
    0xED,
    0x22,
    0x07,
    0xD3,
    0x18,
    0x34,
    0x98,
    0xA9,
    0x30,
    0x56,
    0x22,
    0x1F,
    0xDD,
    0xAC,
    0x7E,
    0x0C,
    0x0E,
    0x83,
    0x6D,
    0x34,
    0x38,
    0x4C,
    0x4F,
    0xE9,
    0x46,
    0x21,
    0x46,
    0x83,
    0xC4,
    0xBA,
    0x8C,
    0xC2,
    0x25,
    0x0A,
    0xB5,
    0x2C,
    0xD0,
    0x33,
    0x25,
    0x87,
    0x3C,
    0x49,
    0x38,
    0x93,
    0xAA,
    0x80,
    0x48,
    0xA3,
    0xDC,
    0x81,
    0x42,
    0xE1,
    0x65,
    0x17,
    0xA1,
    0xB3,
    0xF3,
    0xCE,
    0x4E,
    0x6C,
    0xB5,
    0xC8,
    0xA4,
    0x38,
    0x63,
    0x61,
    0xB1,
    0x28,
    0xC4,
    0xF1,
    0x49,
    0xE7,
    0x40,
    0x88,
    0x2B,
    0xA6,
    0xCE,
    0x96,
    0x39,
    0x21,
    0xD6,
    0x10,
    0xA1,
    0x66,
    0xB4,
    0x35,
    0xE4,
    0x01,
    0xEB,
    0x44,
    0xF6,
    0xB2,
    0x80,
    0x2C,
    0xB0,
    0xB0,
    0x19,
    0xB6,
    0x34,
    0xAA,
    0xF7,
    0xC8,
    0xAF,
    0x32,
    0x79,
    0xE9,
    0x1F,
    0x79,
    0x8E,
    0xAD,
    0xF1,
    0xCE,
    0x6A,
    0xCE,
    0x32,
    0x58,
    0x68,
    0x34,
    0x53,
    0x9F,
    0x0E,
    0xF9,
    0xE9,
    0x11,
    0x67,
    0x7E,
    0x99,
    0xE3,
    0x90,
    0x7B,
    0x5C,
    0x78,
    0xCE,
    0x82,
    0xC3,
    0x36,
    0x20,
    0x11,
    0xDF,
    0xAC,
    0x73,
    0x0D,
    0x31,
    0xA6,
    0x56,
    0x4B,
    0x24,
    0x44,
    0xB8,
    0x38,
    0x67,
    0x0F,
    0xCA,
    0xC7,
    0x29,
    0x1A,
    0xCE,
    0x1C,
    0x7E,
    0x2F,
    0x95,
    0x43,
    0x79,
    0x58,
    0xE7,
    0x6F,
    0xF7,
    0xDE,
    0x4D,
    0x57,
    0x62,
    0xAD,
    0xA7,
    0xCA,
    0x15,
    0x30,
    0xC3,
    0x90,
    0x82,
    0xA8,
    0xF4,
    0xDE,
    0x9A,
    0x1A,
    0x47,
    0xF5,
    0x40,
    0x5E,
    0xAD,
    0x89,
    0xB5,
    0x8A,
    0x1F,
    0x86,
    0x3C,
    0x9C,
    0xBA,
    0x21,
    0x10,
    0xAF,
    0xDF,
    0xF0,
    0xD6,
    0x87,
    0x37,
    0x8C,
    0xAE,
    0x8D,
    0x36,
    0x76,
    0x33,
    0xB3,
    0xF6,
    0xFD,
    0xA1,
    0x04,
    0x27,
    0xC1,
    0xB5,
    0xC9,
    0x59,
    0x3B,
    0xA7,
    0x90,
    0x35,
    0xB8,
    0xA6,
    0xC4,
    0x61,
    0xA7,
    0xAE,
    0xFA,
    0xAE,
    0xC8,
    0xF6,
    0x0A,
    0xAB,
    0x37,
    0xEE,
    0xB2,
    0xF0,
    0x98,
    0xF1,
    0xD1,
    0x9D,
    0x0A,
    0xBF,
    0xB0,
    0x57,
    0x5D,
    0xFD,
    0x29,
    0x69,
    0x04,
    0x51,
    0xF8,
    0xA5,
    0xA6,
    0x5C,
    0x10,
    0xB9,
    0xA9,
    0x32,
    0x42,
    0x63,
    0xE2,
    0xCF,
    0xD9,
    0xF1,
    0x51,
    0xBE,
    0xB8,
    0xE0,
    0x5D,
    0x79,
    0x6D,
    0x0E,
    0xE7,
    0x20,
    0xA5,
    0x32,
    0xD3,
    0xE6,
    0x60,
    0x47,
    0x75,
    0xA7,
    0x1B,
    0xD5,
    0xD5,
    0xB0,
    0x9B,
    0xF7,
    0x12,
    0x92,
    0x85,
    0x98,
    0xA3,
    0x9A,
    0xA6,
    0x14,
    0x24,
    0xB2,
    0xBA,
    0xA2,
    0x3B,
    0x4E,
    0x55,
    0xCE,
    0x26,
    0x57,
    0xAC,
    0xC6,
    0xCF,
    0xAA,
    0x37,
    0x2A,
    0x3D,
    0xE8,
    0x48,
    0x8B,
    0x98,
    0xAC,
    0xBC,
    0x79,
    0x3F,
    0x82,
    0xF8,
    0x21,
    0x88,
    0xD6,
    0x48,
    0x41,
    0x21,
    0xAC,
    0x3B,
    0x67,
    0xAF,
    0xDE,
    0x9E,
    0x45,
    0xF2,
    0x1D,
    0x5C,
    0xD4,
    0x8F,
    0xF3,
    0x54,
    0x79,
    0xBC,
    0x68,
    0xE1,
    0x9D,
    0xE4,
    0x8B,
    0x0A,
    0x62,
    0xBB,
    0xB8,
    0x58,
    0x37,
    0xAF,
    0x70,
    0x20,
    0x55,
    0x59,
    0x34,
    0xF8,
    0x47,
    0x07,
    0x07,
    0x07,
    0x55,
    0xF4,
    0xB6,
    0x5C,
    0xFF,
    0x83,
    0x2C,
    0xEB,
    0x65,
    0xFB,
    0xE9,
    0x72,
    0xFC,
    0x98,
    0xE9,
    0x0B,
    0x73,
    0x6B,
    0x13,
    0x99,
    0x41,
    0xBC,
    0x45,
    0xF4,
    0xC9,
    0x9D,
    0xB3,
    0xAF,
    0x69,
    0xDA,
    0x92,
    0x6A,
    0x0B,
    0x44,
    0xCF,
    0x05,
    0x96,
    0x5B,
    0x9D,
    0x44,
    0xBB,
    0xF2,
    0x0A,
    0x13,
    0x28,
    0xB5,
    0x2F,
    0x7A,
    0xBA,
    0x69,
    0x0E,
    0xCE,
    0x10,
    0x8B,
    0x3E,
    0x55,
    0x3B,
    0xA4,
    0x26,
    0x11,
    0x09,
    0xC4,
    0xDE,
    0x86,
    0x76,
    0x1B,
    0x83,
    0x73,
    0x38,
    0x05,
    0xC7,
    0xC6,
    0xD6,
    0x24,
    0x6A,
    0x5A,
    0x3A,
    0x58,
    0xFD,
    0x5E,
    0xFD,
    0xB2,
    0x4C,
    0x22,
    0xBB,
    0x5E,
    0xFD,
    0x8C,
    0x9C,
    0x8A,
    0xA1,
    0xD3,
    0x72,
    0x7F,
    0xED,
    0x72,
    0x87,
    0x11,
    0x51,
    0xEB,
    0x41,
    0x25,
    0x29,
    0x3E,
    0xBA,
    0x3D,
    0xA0,
    0x3C,
    0x38,
    0xCF,
    0x47,
    0xB7,
    0xA8,
    0x8C,
    0x8A,
    0x55,
    0xDB,
    0xE7,
    0xAC,
    0xDB,
    0xE8,
    0x75,
    0x73,
    0xAF,
    0x1F,
    0x5E,
    0x66,
    0xC8,
    0xF4,
    0x01,
    0x2C,
    0x73,
    0x09,
    0x9E,
    0x2C,
    0x97,
    0xF7,
    0x5F,
    0x2F,
    0x3F,
    0x4E,
    0xBE,
    0x5D,
    0xDE,
    0xB2,
    0x3D,
    0x9F,
    0x03,
    0xE6,
    0x6C,
    0x28,
    0x5B,
    0x58,
    0xD2,
    0xD7,
    0x19,
    0x7D,
    0x6A,
    0x49,
    0x1F,
    0x5F,
    0x3E,
    0xDF,
    0xDD,
    0x73,
    0x46,
    0x29,
    0x57,
    0xD6,
    0x0C,
    0xF9,
    0x61,
    0xED,
    0x8C,
    0xA1,
    0x89,
    0xD7,
    0x29,
    0xCC,
    0xA8,
    0x8C,
    0x2A,
    0xA7,
    0x04,
    0x54,
    0xE4,
    0x04,
    0x59,
    0x81,
    0x6F,
    0x4D,
    0xAD,
    0x7F,
    0x18,
    0x52,
    0xCF,
    0x99,
    0x4D,
    0x89,
    0x72,
    0x19,
    0x09,
    0x07,
    0x27,
    0x61,
    0xDC,
    0x5C,
    0xAB,
    0xFE,
    0x44,
    0x18,
    0x9C,
    0x8B,
    0xA4,
    0xB2,
    0xBD,
    0x2F,
    0x28,
    0x40,
    0x4A,
    0x09,
    0xB5,
    0x2C,
    0x51,
    0x34,
    0x3E,
    0x94,
    0x23,
    0xD5,
    0x00,
    0x33,
    0x76,
    0x06,
    0x6C,
    0x86,
    0xAE,
    0x20,
    0x15,
    0xED,
    0x99,
    0x64,
    0x4A,
    0xF6,
    0x04,
    0x6B,
    0x06,
    0x56,
    0x93,
    0x9C,
    0x75,
    0x66,
    0xAA,
    0x50,
    0xCF,
    0x9D,
    0x46,
    0x3B,
    0x3D,
    0x55,
    0x94,
    0x51,
    0xA6,
    0xFC,
    0xBE,
    0x99,
    0xD3,
    0xFB,
    0xA7,
    0x42,
    0x19,
    0x52,
    0xA4,
    0xD6,
    0x15,
    0x57,
    0x11,
    0xE8,
    0x50,
    0xE9,
    0xC8,
    0xEF,
    0xA4,
    0xDA,
    0x06,
    0xD7,
    0x52,
    0x6E,
    0x74,
    0xFA,
    0x84,
    0x81,
    0xB4,
    0x36,
    0xFC,
    0x01,
    0x00,
    0x53,
    0xF7,
    0x14,
    0x19,
    0x09,
    0x00,
    0x00,
};