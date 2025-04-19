#pragma once

/*
As per UEFI Specification, version 2.8

You may use this code for any purpose.

This code is provided as-is and includes no claims, warranties, or
representations whether express, implied, or otherwise for
merchantability, fitness for a particular purpose, non-infringement,
absence of latent or other defects, accuracy, or the presence or
absence of errors, whether or not known or discoverable.

*/

// PE32+ Subsystem type for EFI images
#define EFI_IMAGE_SUBSYSTEM_EFI_APPLICATION 10
#define EFI_IMAGE_SUBSYSTEM_EFI_BOOT_SERVICE_DRIVER 11
#define EFI_IMAGE_SUBSYSTEM_EFI_RUNTIME_DRIVER 12

// PE32+ Machine type for EFI images
#define EFI_IMAGE_MACHINE_IA32 0x014c
#define EFI_IMAGE_MACHINE_IA64 0x0200
#define EFI_IMAGE_MACHINE_EBC 0x0EBC
#define EFI_IMAGE_MACHINE_x64 0x8664
#define EFI_IMAGE_MACHINE_ARMTHUMB_MIXED 0x01C2
#define EFI_IMAGE_MACHINE_AARCH64 0xAA64
#define EFI_IMAGE_MACHINE_RISCV32 0x5032
#define EFI_IMAGE_MACHINE_RISCV64 0x5064
#define EFI_IMAGE_MACHINE_RISCV128 0x5128

/******************/
/*  Status_Codes  */
/******************/
#define EFI_SUCCESS 0x0000000000000000

#define EFI_WARN_UNKNOWN_GLYPH 0x0000000000000001
#define EFI_WARN_DELETE_FAILURE 0x0000000000000002
#define EFI_WARN_WRITE_FAILURE 0x0000000000000003
#define EFI_WARN_BUFFER_TOO_SMALL 0x0000000000000004
#define EFI_WARN_STALE_DATA 0x0000000000000005
#define EFI_WARN_FILE_SYSTEM 0x0000000000000006
#define EFI_WARN_RESET_REQUIRED 0x0000000000000007

#define EFI_LOAD_ERROR 0x8000000000000001
#define EFI_INVALID_PARAMETER 0x8000000000000002
#define EFI_UNSUPPORTED 0x8000000000000003
#define EFI_BAD_BUFFER_SIZE 0x8000000000000004
#define EFI_BUFFER_TOO_SMALL 0x8000000000000005
#define EFI_NOT_READY 0x8000000000000006
#define EFI_DEVICE_ERROR 0x8000000000000007
#define EFI_WRITE_PROTECTED 0x8000000000000008
#define EFI_OUT_OF_RESOURCES 0x8000000000000009
#define EFI_VOLUME_CORRUPTED 0x800000000000000a
#define EFI_VOLUME_FULL 0x800000000000000b
#define EFI_NO_MEDIA 0x800000000000000c
#define EFI_MEDIA_CHANGED 0x800000000000000d
#define EFI_NOT_FOUND 0x800000000000000e
#define EFI_ACCESS_DENIED 0x800000000000000f
#define EFI_NO_RESPONSE 0x8000000000000010
#define EFI_NO_MAPPING 0x8000000000000011
#define EFI_TIMEOUT 0x8000000000000012
#define EFI_NOT_STARTED 0x8000000000000013
#define EFI_ALREADY_STARTED 0x8000000000000014
#define EFI_ABORTED 0x8000000000000015
#define EFI_ICMP_ERROR 0x8000000000000016
#define EFI_TFTP_ERROR 0x8000000000000017
#define EFI_PROTOCOL_ERROR 0x8000000000000018
#define EFI_INCOMPATIBLE_VERSION 0x8000000000000019
#define EFI_SECURITY_VIOLATION 0x800000000000001a
#define EFI_CRC_ERROR 0x800000000000001b
#define EFI_END_OF_MEDIA 0x800000000000001c
#define EFI_END_OF_FILE 0x800000000000001f
#define EFI_INVALID_LANGUAGE 0x8000000000000020
#define EFI_COMPROMISED_DATA 0x8000000000000021
#define EFI_IP_ADDRESS_CONFLICT 0x8000000000000022
#define EFI_HTTP_ERROR 0x8000000000000023

#define EFI_ERROR(status) ((status) & 0x8000000000000000)

#include <stdint.h>
#include <stddef.h>

typedef uint8_t BOOLEAN;
#define TRUE 1
#define FALSE 0

typedef uintmax_t UINTN;
typedef intmax_t INTN;

typedef uint64_t UINT64;
typedef uint32_t UINT32;
typedef uint16_t UINT16;
typedef uint8_t UINT8;

typedef int64_t INT64;
typedef int32_t INT32;
typedef int16_t INT16;
typedef int8_t INT8;

typedef uint8_t CHAR8;
typedef uint16_t CHAR16;

typedef void VOID;

typedef UINTN EFI_STATUS;

typedef VOID *EFI_HANDLE;
typedef VOID *EFI_EVENT;
typedef UINT64 EFI_LBA;
typedef UINTN EFI_TPL;

typedef UINT64 EFI_PHYSICAL_ADDRESS;

typedef UINT8 EFI_MAC_ADDRESS[4];
typedef UINT8 EFI_IPv4_ADDRESS[4];
typedef UINT8 EFI_IPv6_ADDRESS[16];
typedef UINT8 __attribute__((aligned(4))) EFI_IP_ADDRESS[16];

#define IN
#define OUT
#define OPTIONAL
#define CONST const

// Only true for x64(as far as I know)
#define EFIAPI __attribute__((ms_abi))

/*******************/
/*  Boot_Services  */
/*******************/

//
// CreateEvent
//
typedef VOID *EFI_EVENT;
// These types can be “ORed” together as needed – for example,
// EVT_TIMER might be “ORed” with EVT_NOTIFY_WAIT or
// EVT_NOTIFY_SIGNAL. For actual definitions check the specification.
#define EVT_TIMER 0x80000000
#define EVT_RUNTIME 0x40000000
#define EVT_NOTIFY_WAIT 0x00000100

#define EVT_NOTIFY_SIGNAL 0x00000200
#define EVT_SIGNAL_EXIT_BOOT_SERVICES 0x00000201
#define EVT_SIGNAL_VIRTUAL_ADDRESS_CHANGE 0x60000202

typedef VOID(EFIAPI *EFI_EVENT_NOTIFY)(IN EFI_EVENT Event, IN VOID *Context);

typedef EFI_STATUS(EFIAPI *EFI_CREATE_EVENT)(
	IN UINT32 Type, IN EFI_TPL NotifyTpl,
	IN EFI_EVENT_NOTIFY NotifyFunction OPTIONAL,
	IN VOID *NotifyContext OPTIONAL, OUT EFI_EVENT *Event);

//
// CreateEventEx
//
typedef struct {
	UINT32 Data1;
	UINT16 Data2;
	UINT16 Data3;
	UINT8 Data4[8];
} EFI_GUID;
typedef EFI_STATUS(EFIAPI *EFI_CREATE_EVENT_EX)(
	IN UINT32 Type, IN EFI_TPL NotifyTpl,
	IN EFI_EVENT_NOTIFY NotifyFunction OPTIONAL,
	IN CONST VOID *NotifyContext OPTIONAL,
	IN CONST EFI_GUID *EventGroup OPTIONAL, OUT EFI_EVENT *Event);
#define EFI_EVENT_GROUP_EXIT_BOOT_SERVICES \
	{ 0x27abf055,                          \
	  0xb1b8,                              \
	  0x4c26,                              \
	  { 0x80, 0x48, 0x74, 0x8f, 0x37, 0xba, 0xa2, 0xdf } }

#define EFI_EVENT_GROUP_VIRTUAL_ADDRESS_CHANGE \
	{ 0x13fa7698,                              \
	  0xc831,                                  \
	  0x49c7,                                  \
	  { 0x87, 0xea, 0x8f, 0x43, 0xfc, 0xc2, 0x51, 0x96 } }

#define EFI_EVENT_GROUP_MEMORY_MAP_CHANGE \
	{ 0x78bee926,                         \
	  0x692f,                             \
	  0x48fd,                             \
	  { 0x9e, 0xdb, 0x1, 0x42, 0x2e, 0xf0, 0xd7, 0xab } }

#define EFI_EVENT_GROUP_READY_TO_BOOT \
	{ 0x7ce88fb3,                     \
	  0x4bd7,                         \
	  0x4679,                         \
	  { 0x87, 0xa8, 0xa8, 0xd8, 0xde, 0xe5, 0xd, 0x2b } }

#define EFI_EVENT_GROUP_RESET_SYSTEM \
	{ 0x62da6a56,                    \
	  0x13fb,                        \
	  0x485a,                        \
	  { 0xa8, 0xda, 0xa3, 0xdd, 0x79, 0x12, 0xcb, 0x6b } }

//
// CloseEvent
//
typedef EFI_STATUS(EFIAPI *EFI_CLOSE_EVENT)(IN EFI_EVENT Event);

//
// SignalEvent
//
typedef EFI_STATUS(EFIAPI *EFI_SIGNAL_EVENT)(IN EFI_EVENT Event);

//
// WaitForEvent
//
typedef EFI_STATUS(EFIAPI *EFI_WAIT_FOR_EVENT)(IN UINTN NumberOfEvents,
											   IN EFI_EVENT *Event,
											   OUT UINTN *Index);

//
// CheckEvent
//
typedef EFI_STATUS(EFIAPI *EFI_CHECK_EVENT)(IN EFI_EVENT Event);

//
// SetTimer
//
typedef enum { TimerCancel, TimerPeriodic, TimerRelative } EFI_TIMER_DELAY;
typedef EFI_STATUS(EFIAPI *EFI_SET_TIMER)(IN EFI_EVENT Event,
										  IN EFI_TIMER_DELAY Type,
										  IN UINT64 TriggerTime);

//
// RaiseTPL
//
typedef EFI_TPL(EFIAPI *EFI_RAISE_TPL)(IN EFI_TPL NewTpl);
typedef UINTN EFI_TPL;
#define TPL_APPLICATION 4
#define TPL_CALLBACK 8
#define TPL_NOTIFY 16
#define TPL_HIGH_LEVEL 31

//
// RestoreTPL
//
typedef VOID(EFIAPI *EFI_RESTORE_TPL)(IN EFI_TPL OldTpl);

//
// AllocatePages
//
typedef enum {
	AllocateAnyPages,
	AllocateMaxAddress,
	AllocateAddress,
	MaxAllocateType
} EFI_ALLOCATE_TYPE;
typedef enum {
	EfiReservedMemoryType = 0,
	EfiLoaderCode = 1,
	EfiLoaderData = 2,
	EfiBootServicesCode = 3,
	EfiBootServicesData = 4,
	EfiRuntimeServicesCode = 5,
	EfiRuntimeServicesData = 6,
	EfiConventionalMemory = 7,
	EfiUnusableMemory = 8,
	EfiACPIReclaimMemory = 9,
	EfiACPIMemoryNVS = 10,
	EfiMemoryMappedIO = 11,
	EfiMemoryMappedIOPortSpace = 12,
	EfiPalCode = 13,
	EfiPersistentMemory = 14,
	EfiMaxMemoryType = 15
} EFI_MEMORY_TYPE;
typedef EFI_STATUS(EFIAPI *EFI_ALLOCATE_PAGES)(
	IN EFI_ALLOCATE_TYPE Type, IN EFI_MEMORY_TYPE MemoryType, IN UINTN Pages,
	IN OUT EFI_PHYSICAL_ADDRESS *Memory);

//
// FreePages
//
typedef EFI_STATUS(EFIAPI *EFI_FREE_PAGES)(IN EFI_PHYSICAL_ADDRESS Memory,
										   IN UINTN Pages);

//
// GetMemoryMap
//
typedef UINT64 EFI_VIRTUAL_ADDRESS;
typedef struct {
	UINT32 Type;
	EFI_PHYSICAL_ADDRESS PhysicalStart;
	EFI_VIRTUAL_ADDRESS VirtualStart;
	UINT64 NumberOfPages;
	UINT64 Attribute;
} EFI_MEMORY_DESCRIPTOR;
typedef EFI_STATUS(EFIAPI *EFI_GET_MEMORY_MAP)(
	IN OUT UINTN *MemoryMapSize, IN OUT EFI_MEMORY_DESCRIPTOR *MemoryMap,
	OUT UINTN *MapKey, OUT UINTN *DescriptorSize,
	OUT UINT32 *DescriptorVersion);
#define EFI_MEMORY_UC 0x0000000000000001
#define EFI_MEMORY_WC 0x0000000000000002
#define EFI_MEMORY_WT 0x0000000000000004
#define EFI_MEMORY_WB 0x0000000000000008
#define EFI_MEMORY_UCE 0x0000000000000010
#define EFI_MEMORY_WP 0x0000000000001000
#define EFI_MEMORY_RP 0x0000000000002000
#define EFI_MEMORY_XP 0x0000000000004000
#define EFI_MEMORY_NV 0x0000000000008000
#define EFI_MEMORY_MORE_RELIABLE 0x0000000000010000
#define EFI_MEMORY_RO 0x0000000000020000
#define EFI_MEMORY_SP 0x0000000000040000
#define EFI_MEMORY_CPU_CRYPTO 0x0000000000080000
#define EFI_MEMORY_RUNTIME 0x8000000000000000
#define EFI_MEMORY_DESCRIPTOR_VERSION 1

//
// AllocatePool
//
typedef EFI_STATUS(EFIAPI *EFI_ALLOCATE_POOL)(IN EFI_MEMORY_TYPE PoolType,
											  IN UINTN Size, OUT VOID **Buffer);

//
// FreePool
//
typedef EFI_STATUS(EFIAPI *EFI_FREE_POOL)(IN VOID *Buffer);

//
// InstallProtocolInterface
//
typedef enum { EFI_NATIVE_INTERFACE } EFI_INTERFACE_TYPE;
typedef EFI_STATUS(EFIAPI *EFI_INSTALL_PROTOCOL_INTERFACE)(
	IN OUT EFI_HANDLE *Handle, IN EFI_GUID *Protocol,
	IN EFI_INTERFACE_TYPE InterfaceType, IN VOID *Interface);

//
// UninstallProtocolInterface
//
typedef EFI_STATUS(EFIAPI *EFI_UNINSTALL_PROTOCOL_INTERFACE)(
	IN EFI_HANDLE Handle, IN EFI_GUID *Protocol, IN VOID *Interface);

//
// ReinstallProtocolInterface
//
typedef EFI_STATUS(EFIAPI *EFI_REINSTALL_PROTOCOL_INTERFACE)(
	IN EFI_HANDLE Handle, IN EFI_GUID *Protocol, IN VOID *OldInterface,
	IN VOID *NewInterface);

//
// RegisterProtocolNotify
//
typedef EFI_STATUS(EFIAPI *EFI_REGISTER_PROTOCOL_NOTIFY)(
	IN EFI_GUID *Protocol, IN EFI_EVENT Event, OUT VOID **Registration);

//
// LocateHandle
//
typedef enum {
	AllHandles,
	ByRegisterNotify,
	ByProtocol
} EFI_LOCATE_SEARCH_TYPE;
typedef EFI_STATUS(EFIAPI *EFI_LOCATE_HANDLE)(
	IN EFI_LOCATE_SEARCH_TYPE SearchType, IN EFI_GUID *Protocol OPTIONAL,
	IN VOID *SearchKey OPTIONAL, IN OUT UINTN *BufferSize,
	OUT EFI_HANDLE *Buffer);

//
// HandleProtocol
//
typedef EFI_STATUS(EFIAPI *EFI_HANDLE_PROTOCOL)(IN EFI_HANDLE Handle,
												IN EFI_GUID *Protocol,
												OUT VOID **Interface);

//
// LocateDevicePath
//
typedef struct {
	UINT8 Type;
	UINT8 SubType;
	UINT8 Length[2];
} EFI_DEVICE_PATH_PROTOCOL;
typedef EFI_STATUS(EFIAPI *EFI_LOCATE_DEVICE_PATH)(
	IN EFI_GUID *Protocol, IN OUT EFI_DEVICE_PATH_PROTOCOL **DevicePath,
	OUT EFI_HANDLE *Device);

//
// OpenProtocol
//
typedef EFI_STATUS(EFIAPI *EFI_OPEN_PROTOCOL)(IN EFI_HANDLE Handle,
											  IN EFI_GUID *Protocol,
											  OUT VOID **InterfaceOPTIONAL,
											  IN EFI_HANDLE AgentHandle,
											  IN EFI_HANDLE ControllerHandle,
											  IN UINT32 Attributes);
#define EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL 0x00000001
#define EFI_OPEN_PROTOCOL_GET_PROTOCOL 0x00000002
#define EFI_OPEN_PROTOCOL_TEST_PROTOCOL 0x00000004
#define EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER 0x00000008
#define EFI_OPEN_PROTOCOL_BY_DRIVER 0x00000010
#define EFI_OPEN_PROTOCOL_EXCLUSIVE 0x00000020

//
// CloseProtocol
//
typedef EFI_STATUS(EFIAPI *EFI_CLOSE_PROTOCOL)(IN EFI_HANDLE Handle,
											   IN EFI_GUID *Protocol,
											   IN EFI_HANDLE AgentHandle,
											   IN EFI_HANDLE ControllerHandle);

//
// OpenProtocolInformation
//
typedef struct {
	EFI_HANDLE AgentHandle;
	EFI_HANDLE ControllerHandle;
	UINT32 Attributes;
	UINT32 OpenCount;
} EFI_OPEN_PROTOCOL_INFORMATION_ENTRY;
typedef EFI_STATUS(EFIAPI *EFI_OPEN_PROTOCOL_INFORMATION)(
	IN EFI_HANDLE Handle, IN EFI_GUID *Protocol,
	OUT EFI_OPEN_PROTOCOL_INFORMATION_ENTRY **EntryBuffer,
	OUT UINTN *EntryCount);

//
// ConnectController
//
typedef EFI_STATUS(EFIAPI *EFI_CONNECT_CONTROLLER)(
	IN EFI_HANDLE ControllerHandle, IN EFI_HANDLE *DriverImageHandle OPTIONAL,
	IN EFI_DEVICE_PATH_PROTOCOL *RemainingDevicePath OPTIONAL,
	IN BOOLEAN Recursive);

//
// DisconnectController
//
typedef EFI_STATUS(EFIAPI *EFI_DISCONNECT_CONTROLLER)(
	IN EFI_HANDLE ControllerHandle, IN EFI_HANDLE DriverImageHandle OPTIONAL,
	IN EFI_HANDLE ChildHandle OPTIONAL);

//
// ProtocolsPerHandle
//
typedef EFI_STATUS(EFIAPI *EFI_PROTOCOLS_PER_HANDLE)(
	IN EFI_HANDLE Handle, OUT EFI_GUID ***ProtocolBuffer,
	OUT UINTN *ProtocolBufferCount);

//
// LocateHandleBuffer
//
typedef EFI_STATUS(EFIAPI *EFI_LOCATE_HANDLE_BUFFER)(
	IN EFI_LOCATE_SEARCH_TYPE SearchType, IN EFI_GUID *Protocol OPTIONAL,
	IN VOID *SearchKeyOPTIONAL, IN OUT UINTN *NoHandles,
	OUT EFI_HANDLE **Buffer);

//
// LocateProtocol
//
typedef EFI_STATUS(EFIAPI *EFI_LOCATE_PROTOCOL)(IN EFI_GUID *Protocol,
												IN VOID *Registration OPTIONAL,
												OUT VOID **Interface);

//
// InstallMultipleProtocolInterfaces
//
typedef EFI_STATUS(EFIAPI *EFI_INSTALL_MULTIPLE_PROTOCOL_INTERFACES)(
	IN OUT EFI_HANDLE *Handle, ...);

//
// UninstallMultipleProtocolInterfaces
//
typedef EFI_STATUS(EFIAPI *EFI_UNINSTALL_MULTIPLE_PROTOCOL_INTERFACES)(
	IN EFI_HANDLE Handle, ...);

//
// LoadImage
//
typedef EFI_STATUS(EFIAPI *EFI_IMAGE_LOAD)(
	IN BOOLEAN BootPolicy, IN EFI_HANDLE ParentImageHandle,
	IN EFI_DEVICE_PATH_PROTOCOL *DevicePath, IN VOID *SourceBuffer OPTIONAL,
	IN UINTN SourceSize, OUT EFI_HANDLE *ImageHandle);

//
// StartImage
//
typedef EFI_STATUS(EFIAPI *EFI_IMAGE_START)(IN EFI_HANDLE ImageHandle,
											OUT UINTN *ExitDataSize,
											OUT CHAR16 **ExitData OPTIONAL);

//
// UnloadImage
//
typedef EFI_STATUS(EFIAPI *EFI_IMAGE_UNLOAD)(IN EFI_HANDLE ImageHandle);

//
// Exit
//
typedef EFI_STATUS(EFIAPI *EFI_EXIT)(IN EFI_HANDLE ImageHandle,
									 IN EFI_STATUS ExitStatus,
									 IN UINTN ExitDataSize,
									 IN CHAR16 *ExitData OPTIONAL);

//
// ExitBootServices
//
typedef EFI_STATUS(EFIAPI *EFI_EXIT_BOOT_SERVICES)(IN EFI_HANDLE ImageHandle,
												   IN UINTN MapKey);

//
// SetWatchdogTimer
//
typedef EFI_STATUS(EFIAPI *EFI_SET_WATCHDOG_TIMER)(
	IN UINTN Timeout, IN UINT64 WatchdogCode, IN UINTN DataSize,
	IN CHAR16 *WatchdogData OPTIONAL);

//
// Stall
//
typedef EFI_STATUS(EFIAPI *EFI_STALL)(IN UINTN Microseconds);

//
// CopyMem
//
typedef VOID(EFIAPI *EFI_COPY_MEM)(IN VOID *Destination, IN VOID *Source,
								   IN UINTN Length);

//
// SetMem
//
typedef VOID(EFIAPI *EFI_SET_MEM)(IN VOID *Buffer, IN UINTN Size,
								  IN UINT8 Value);

//
// GetNextMonotonicCount
//
typedef EFI_STATUS(EFIAPI *EFI_GET_NEXT_MONOTONIC_COUNT)(OUT UINT64 *Count);

//
// InstallConfigurationTable
//
typedef EFI_STATUS(EFIAPI *EFI_INSTALL_CONFIGURATION_TABLE)(IN EFI_GUID *Guid,
															IN VOID *Table);

//
// CalculateCrc32
//
typedef EFI_STATUS(EFIAPI *EFI_CALCULATE_CRC32)(IN VOID *Data,
												IN UINTN DataSize,
												OUT UINT32 *Crc32);

/**********************/
/*  Runtime_Services  */
/**********************/
#define EFI_RT_SUPPORTED_GET_TIME 0x0001
#define EFI_RT_SUPPORTED_SET_TIME 0x0002
#define EFI_RT_SUPPORTED_GET_WAKEUP_TIME 0x0004
#define EFI_RT_SUPPORTED_SET_WAKEUP_TIME 0x0008
#define EFI_RT_SUPPORTED_GET_VARIABLE 0x0010
#define EFI_RT_SUPPORTED_GET_NEXT_VARIABLE_NAME 0x0020
#define EFI_RT_SUPPORTED_SET_VARIABLE 0x0040
#define EFI_RT_SUPPORTED_SET_VIRTUAL_ADDRESS_MAP 0x0080
#define EFI_RT_SUPPORTED_CONVERT_POINTER 0x0100
#define EFI_RT_SUPPORTED_GET_NEXT_HIGH_MONOTONIC_COUNT 0x0200
#define EFI_RT_SUPPORTED_RESET_SYSTEM 0x0400
#define EFI_RT_SUPPORTED_UPDATE_CAPSULE 0x0800
#define EFI_RT_SUPPORTED_QUERY_CAPSULE_CAPABILITIES 0x1000
#define EFI_RT_SUPPORTED_QUERY_VARIABLE_INFO 0x2000

//
// GetVariable
//
typedef EFI_STATUS(EFIAPI *EFI_GET_VARIABLE)(IN CHAR16 *VariableName,
											 IN EFI_GUID *VendorGuid,
											 OUT UINT32 *Attributes OPTIONAL,
											 IN OUT UINTN *DataSize,
											 OUT VOID *Data OPTIONAL);
#define EFI_VARIABLE_NON_VOLATILE 0x00000001
#define EFI_VARIABLE_BOOTSERVICE_ACCESS 0x00000002
#define EFI_VARIABLE_RUNTIME_ACCESS 0x00000004
#define EFI_VARIABLE_HARDWARE_ERROR_RECORD 0x00000008
#define EFI_VARIABLE_AUTHENTICATED_WRITE_ACCESS 0x00000010
// NOTE: EFI_VARIABLE_AUTHENTICATED_WRITE_ACCESS is deprecated
// and should be considered reserved.
#define EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS 0x00000020
#define EFI_VARIABLE_APPEND_WRITE 0x00000040
#define EFI_VARIABLE_ENHANCED_AUTHENTICATED_ACCESS 0x00000080
#define EFI_VARIABLE_AUTHENTICATION_3_CERT_ID_SHA256 1
typedef struct {
	UINT8 Type;
	UINT32 IdSize;
	// UINT8      Id[IdSize];
} EFI_VARIABLE_AUTHENTICATION_3_CERT_ID;

//
// GetNextVariableName
//
typedef EFI_STATUS(EFIAPI *EFI_GET_NEXT_VARIABLE_NAME)(
	IN OUT UINTN *VariableNameSize, IN OUT CHAR16 *VariableName,
	IN OUT EFI_GUID *VendorGuid);

//
// SetVariable
//
typedef EFI_STATUS(EFIAPI *EFI_SET_VARIABLE)(IN CHAR16 *VariableName,
											 IN EFI_GUID *VendorGuid,
											 IN UINT32 Attributes,
											 IN UINTN DataSize, IN VOID *Data);

//
// QueryVariableInfo
//
typedef EFI_STATUS(EFIAPI *EFI_QUERY_VARIABLE_INFO)(
	IN UINT32 Attributes, OUT UINT64 *MaximumVariableStorageSize,
	OUT UINT64 *RemainingVariableStorageSize, OUT UINT64 *MaximumVariableSize);

//
// GetTime
//
// This represents the current time information
typedef struct {
	UINT16 Year; // 1900 – 9999
	UINT8 Month; // 1 – 12
	UINT8 Day; // 1 – 31
	UINT8 Hour; // 0 – 23
	UINT8 Minute; // 0 – 59
	UINT8 Second; // 0 – 59
	UINT8 Pad1;
	UINT32 Nanosecond; // 0 – 999,999,999
	INT16 TimeZone; // -1440 to 1440 or 2047
	UINT8 Daylight;
	UINT8 Pad2;
} EFI_TIME;
// Bit Definitions for EFI_TIME.Daylight
#define EFI_TIME_ADJUST_DAYLIGHT 0x01
#define EFI_TIME_IN_DAYLIGHT 0x02
// Value Definition for EFI_TIME.TimeZone
#define EFI_UNSPECIFIED_TIMEZONE 0x07FF
// This provides the capabilities of the
// real time clock device as exposed through the EFI interfaces.
typedef struct {
	UINT32 Resolution;
	UINT32 Accuracy;
	BOOLEAN SetsToZero;
} EFI_TIME_CAPABILITIES;
typedef EFI_STATUS(EFIAPI *EFI_GET_TIME)(
	OUT EFI_TIME *Time, OUT EFI_TIME_CAPABILITIES *Capabilities OPTIONAL);

//
// SetTime
//
typedef EFI_STATUS(EFIAPI *EFI_SET_TIME)(IN EFI_TIME *Time);

//
// GetWakeupTime
//
typedef EFI_STATUS(EFIAPI *EFI_GET_WAKEUP_TIME)(OUT BOOLEAN *Enabled,
												OUT BOOLEAN *Pending,
												OUT EFI_TIME *Time);

//
// SetWakeupTime
//
typedef EFI_STATUS(EFIAPI *EFI_SET_WAKEUP_TIME)(IN BOOLEAN Enable,
												IN EFI_TIME *Time OPTIONAL);

//
// SetVirtualAddressMap
//
typedef EFI_STATUS(EFIAPI *EFI_SET_VIRTUAL_ADDRESS_MAP)(
	IN UINTN MemoryMapSize, IN UINTN DescriptorSize,
	IN UINT32 DescriptorVersion, IN EFI_MEMORY_DESCRIPTOR *VirtualMap);

//
// ConvertPointer
//
typedef EFI_STATUS(EFIAPI *EFI_CONVERT_POINTER)(IN UINTN DebugDisposition,
												IN VOID **Address);
#define EFI_OPTIONAL_PTR 0x00000001

//
// ResetSystem
//
typedef enum {
	EfiResetCold,
	EfiResetWarm,
	EfiResetShutdown,
	EfiResetPlatformSpecific
} EFI_RESET_TYPE;
typedef VOID(EFIAPI *EFI_RESET_SYSTEM)(IN EFI_RESET_TYPE ResetType,
									   IN EFI_STATUS ResetStatus,
									   IN UINTN DataSize,
									   IN VOID *ResetData OPTIONAL);

//
// GetNextHighMonotonicCount
//
typedef EFI_STATUS(EFIAPI *EFI_GET_NEXT_HIGH_MONOTONIC_COUNT)(
	OUT UINT32 *HighCount);

//
// UpdateCapsule
//
typedef struct {
	UINT64 Length;
	union {
		EFI_PHYSICAL_ADDRESS DataBlock;
		EFI_PHYSICAL_ADDRESS ContinuationPointer;
	} Union;
} EFI_CAPSULE_BLOCK_DESCRIPTOR;
typedef struct {
	EFI_GUID CapsuleGuid;
	UINT32 HeaderSize;
	UINT32 Flags;
	UINT32 CapsuleImageSize;
} EFI_CAPSULE_HEADER;
typedef EFI_STATUS(EFIAPI *EFI_UPDATE_CAPSULE)(
	IN EFI_CAPSULE_HEADER **CapsuleHeaderArray, IN UINTN CapsuleCount,
	IN EFI_PHYSICAL_ADDRESS ScatterGatherList OPTIONAL);

//
// QueryCapsuleCapabilities
//
typedef EFI_STATUS(EFIAPI *EFI_QUERY_CAPSULE_CAPABILITIES)(
	IN EFI_CAPSULE_HEADER **CapsuleHeaderArray, IN UINTN CapsuleCount,
	OUT UINT64 *MaximumCapsuleSize, OUT EFI_RESET_TYPE *ResetType);

/***********************/
/*  Simple_Text_Stuff  */
/***********************/

//
//
// Input stuff
//
//
#define EFI_SIMPLE_TEXT_INPUT_PROTOCOL_GUID \
	{ 0x387477c1,                           \
	  0x69c7,                               \
	  0x11d2,                               \
	  { 0x8e, 0x39, 0x00, 0xa0, 0xc9, 0x69, 0x72, 0x3b } }

//
// InputReset
//
struct EFI_SIMPLE_TEXT_INPUT_PROTOCOL;
typedef EFI_STATUS(EFIAPI *EFI_INPUT_RESET)(
	IN struct EFI_SIMPLE_TEXT_INPUT_PROTOCOL *This,
	IN BOOLEAN ExtendedVerification);

//
// ReadKeyStroke
//
typedef struct {
	UINT16 ScanCode;
	CHAR16 UnicodeChar;
} EFI_INPUT_KEY;
typedef EFI_STATUS(EFIAPI *EFI_INPUT_READ_KEY)(
	IN struct EFI_SIMPLE_TEXT_INPUT_PROTOCOL *This, OUT EFI_INPUT_KEY *Key);

typedef struct EFI_SIMPLE_TEXT_INPUT_PROTOCOL {
	EFI_INPUT_RESET Reset;
	EFI_INPUT_READ_KEY ReadKeyStroke;
	EFI_EVENT WaitForKey;
} EFI_SIMPLE_TEXT_INPUT_PROTOCOL;

//
//
// Output stuff
//
//

//*******************************************************
// UNICODE DRAWING CHARACTERS
//*******************************************************
#define BOXDRAW_HORIZONTAL 0x2500
#define BOXDRAW_VERTICAL 0x2502
#define BOXDRAW_DOWN_RIGHT 0x250c
#define BOXDRAW_DOWN_LEFT 0x2510
#define BOXDRAW_UP_RIGHT 0x2514
#define BOXDRAW_UP_LEFT 0x2518
#define BOXDRAW_VERTICAL_RIGHT 0x251c
#define BOXDRAW_VERTICAL_LEFT 0x2524
#define BOXDRAW_DOWN_HORIZONTAL 0x252c
#define BOXDRAW_UP_HORIZONTAL 0x2534
#define BOXDRAW_VERTICAL_HORIZONTAL 0x253c
#define BOXDRAW_DOUBLE_HORIZONTAL 0x2550
#define BOXDRAW_DOUBLE_VERTICAL 0x2551
#define BOXDRAW_DOWN_RIGHT_DOUBLE 0x2552
#define BOXDRAW_DOWN_DOUBLE_RIGHT 0x2553
#define BOXDRAW_DOUBLE_DOWN_RIGHT 0x2554
#define BOXDRAW_DOWN_LEFT_DOUBLE 0x2555
#define BOXDRAW_DOWN_DOUBLE_LEFT 0x2556
#define BOXDRAW_DOUBLE_DOWN_LEFT 0x2557
#define BOXDRAW_UP_RIGHT_DOUBLE 0x2558
#define BOXDRAW_UP_DOUBLE_RIGHT 0x2559
#define BOXDRAW_DOUBLE_UP_RIGHT 0x255a
#define BOXDRAW_UP_LEFT_DOUBLE 0x255b
#define BOXDRAW_UP_DOUBLE_LEFT 0x255c
#define BOXDRAW_DOUBLE_UP_LEFT 0x255d
#define BOXDRAW_VERTICAL_RIGHT_DOUBLE 0x255e
#define BOXDRAW_VERTICAL_DOUBLE_RIGHT 0x255f
#define BOXDRAW_DOUBLE_VERTICAL_RIGHT 0x2560
#define BOXDRAW_VERTICAL_LEFT_DOUBLE 0x2561
#define BOXDRAW_VERTICAL_DOUBLE_LEFT 0x2562
#define BOXDRAW_DOUBLE_VERTICAL_LEFT 0x2563
#define BOXDRAW_DOWN_HORIZONTAL_DOUBLE 0x2564
#define BOXDRAW_DOWN_DOUBLE_HORIZONTAL 0x2565
#define BOXDRAW_DOUBLE_DOWN_HORIZONTAL 0x2566
#define BOXDRAW_UP_HORIZONTAL_DOUBLE 0x2567
#define BOXDRAW_UP_DOUBLE_HORIZONTAL 0x2568
#define BOXDRAW_DOUBLE_UP_HORIZONTAL 0x2569
#define BOXDRAW_VERTICAL_HORIZONTAL_DOUBLE 0x256a
#define BOXDRAW_VERTICAL_DOUBLE_HORIZONTAL 0x256b
#define BOXDRAW_DOUBLE_VERTICAL_HORIZONTAL 0x256c
//*******************************************************
// EFI Required Block Elements Code Chart
//*******************************************************
#define BLOCKELEMENT_FULL_BLOCK 0x2588
#define BLOCKELEMENT_LIGHT_SHADE 0x2591
//*******************************************************
// EFI Required Geometric Shapes Code Chart
//*******************************************************
#define GEOMETRICSHAPE_UP_TRIANGLE 0x25b2
#define GEOMETRICSHAPE_RIGHT_TRIANGLE 0x25ba
#define GEOMETRICSHAPE_DOWN_TRIANGLE 0x25bc
#define GEOMETRICSHAPE_LEFT_TRIANGLE 0x25c4
//*******************************************************
// EFI Required Arrow shapes
//*******************************************************
#define ARROW_UP 0x2191
#define ARROW_DOWN 0x2193

#define EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL_GUID \
	{ 0x387477c2,                            \
	  0x69c7,                                \
	  0x11d2,                                \
	  { 0x8e, 0x39, 0x00, 0xa0, 0xc9, 0x69, 0x72, 0x3b } }

//
// TestReset
//
struct EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL;
typedef EFI_STATUS(EFIAPI *EFI_TEXT_RESET)(
	IN struct EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *This,
	IN BOOLEAN ExtendedVerification);

//
// OutputString
//
typedef EFI_STATUS(EFIAPI *EFI_TEXT_STRING)(
	IN struct EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *This, IN CHAR16 *String);

//
// TestString
//
typedef EFI_STATUS(EFIAPI *EFI_TEXT_TEST_STRING)(
	IN struct EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *This, IN CHAR16 *String);

//
// QueryMode
//
typedef EFI_STATUS(EFIAPI *EFI_TEXT_QUERY_MODE)(
	IN struct EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *This, IN UINTN ModeNumber,
	OUT UINTN *Columns, OUT UINTN *Rows);

//
// SetMode
//
typedef EFI_STATUS (*EFIAPI EFI_TEXT_SET_MODE)(
	IN struct EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *This, IN UINTN ModeNumber);

//
// SetAttribute
//
typedef EFI_STATUS(EFIAPI *EFI_TEXT_SET_ATTRIBUTE)(
	IN struct EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *This, IN UINTN Attribute);
//*******************************************************
// Attributes
//*******************************************************
#define EFI_BLACK 0x00
#define EFI_BLUE 0x01
#define EFI_GREEN 0x02
#define EFI_CYAN 0x03
#define EFI_RED 0x04
#define EFI_MAGENTA 0x05
#define EFI_BROWN 0x06
#define EFI_LIGHTGRAY 0x07
#define EFI_BRIGHT 0x08
#define EFI_DARKGRAY 0x08
#define EFI_LIGHTBLUE 0x09
#define EFI_LIGHTGREEN 0x0A
#define EFI_LIGHTCYAN 0x0B
#define EFI_LIGHTRED 0x0C
#define EFI_LIGHTMAGENTA 0x0D
#define EFI_YELLOW 0x0E
#define EFI_WHITE 0x0F

#define EFI_BACKGROUND_BLACK 0x00
#define EFI_BACKGROUND_BLUE 0x10
#define EFI_BACKGROUND_GREEN 0x20
#define EFI_BACKGROUND_CYAN 0x30
#define EFI_BACKGROUND_RED 0x40
#define EFI_BACKGROUND_MAGENTA 0x50
#define EFI_BACKGROUND_BROWN 0x60
#define EFI_BACKGROUND_LIGHTGRAY 0x70
// The foreground color and background color can
//  be OR-ed(|) together to set them both in one call.

//
// ClearScreen
//
typedef EFI_STATUS(EFIAPI *EFI_TEXT_CLEAR_SCREEN)(
	IN struct EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *This);

//
// SetCursorPosition
//
typedef EFI_STATUS(EFIAPI *EFI_TEXT_SET_CURSOR_POSITION)(
	IN struct EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *This, IN UINTN Column,
	IN UINTN Row);

//
// EnableCursor
//
typedef EFI_STATUS(EFIAPI *EFI_TEXT_ENABLE_CURSOR)(
	IN struct EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *This, IN BOOLEAN Visible);

typedef struct {
	INT32 MaxMode; // current settings
	INT32 Mode;
	INT32 Attribute;
	INT32 CursorColumn;
	INT32 CursorRow;
	BOOLEAN CursorVisible;
} SIMPLE_TEXT_OUTPUT_MODE;

typedef struct EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL {
	EFI_TEXT_RESET Reset;
	EFI_TEXT_STRING OutputString;
	EFI_TEXT_TEST_STRING TestString;
	EFI_TEXT_QUERY_MODE QueryMode;
	EFI_TEXT_SET_MODE SetMode;
	EFI_TEXT_SET_ATTRIBUTE SetAttribute;
	EFI_TEXT_CLEAR_SCREEN ClearScreen;
	EFI_TEXT_SET_CURSOR_POSITION SetCursorPosition;
	EFI_TEXT_ENABLE_CURSOR EnableCursor;
	SIMPLE_TEXT_OUTPUT_MODE *Mode;
} EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL;

/***********************/
/*  Table_Definitions  */
/***********************/

#define EFI_SPECIFICATION_VERSION EFI_SYSTEM_TABLE_REVISION
#define EFI_SYSTEM_TABLE_REVISION EFI_2_8_SYSTEM_TABLE_REVISION

typedef struct {
	UINT64 Signature;
	UINT32 Revision;
	UINT32 HeaderSize;
	UINT32 CRC32;
	UINT32 Reserved;
} EFI_TABLE_HEADER;

#define EFI_BOOT_SERVICES_SIGNATURE 0x56524553544f4f42
#define EFI_BOOT_SERVICES_REVISION EFI_SPECIFICATION_VERSION
typedef struct {
	EFI_TABLE_HEADER Hdr;

	//
	// Task Priority Services
	//
	EFI_RAISE_TPL RaiseTPL; // EFI 1.0+
	EFI_RESTORE_TPL RestoreTPL; // EFI 1.0+

	//
	// Memory Services
	//
	EFI_ALLOCATE_PAGES AllocatePages; // EFI 1.0+
	EFI_FREE_PAGES FreePages; // EFI 1.0+
	EFI_GET_MEMORY_MAP GetMemoryMap; // EFI 1.0+
	EFI_ALLOCATE_POOL AllocatePool; // EFI 1.0+
	EFI_FREE_POOL FreePool; // EFI 1.0+

	//
	// Event & Timer Services
	//
	EFI_CREATE_EVENT CreateEvent; // EFI 1.0+
	EFI_SET_TIMER SetTimer; // EFI 1.0+
	EFI_WAIT_FOR_EVENT WaitForEvent; // EFI 1.0+
	EFI_SIGNAL_EVENT SignalEvent; // EFI 1.0+
	EFI_CLOSE_EVENT CloseEvent; // EFI 1.0+
	EFI_CHECK_EVENT CheckEvent; // EFI 1.0+

	//
	// Protocol Handler Services
	//
	EFI_INSTALL_PROTOCOL_INTERFACE InstallProtocolInterface; // EFI 1.0+
	EFI_REINSTALL_PROTOCOL_INTERFACE ReinstallProtocolInterface; // EFI 1.0+
	EFI_UNINSTALL_PROTOCOL_INTERFACE UninstallProtocolInterface; // EFI 1.0+
	EFI_HANDLE_PROTOCOL HandleProtocol; // EFI 1.0+
	VOID *Reserved; // EFI 1.0+
	EFI_REGISTER_PROTOCOL_NOTIFY RegisterProtocolNotify; // EFI 1.0+
	EFI_LOCATE_HANDLE LocateHandle; // EFI 1.0+
	EFI_LOCATE_DEVICE_PATH LocateDevicePath; // EFI 1.0+
	EFI_INSTALL_CONFIGURATION_TABLE InstallConfigurationTable; // EFI 1.0+

	//
	// Image Services
	//
	EFI_IMAGE_LOAD LoadImage; // EFI 1.0+
	EFI_IMAGE_START
	StartImage; // EFI 1.0+UEFI Specification, Version 2.8 EFI System Table
	EFI_EXIT Exit; // EFI 1.0+
	EFI_IMAGE_UNLOAD UnloadImage; // EFI 1.0+
	EFI_EXIT_BOOT_SERVICES ExitBootServices; // EFI 1.0+

	//
	// Miscellaneous Services
	//
	EFI_GET_NEXT_MONOTONIC_COUNT GetNextMonotonicCount; // EFI 1.0+
	EFI_STALL Stall; // EFI 1.0+
	EFI_SET_WATCHDOG_TIMER SetWatchdogTimer; // EFI 1.0+

	//
	// DriverSupport Services
	//
	EFI_CONNECT_CONTROLLER ConnectController; // EFI 1.1
	EFI_DISCONNECT_CONTROLLER DisconnectController; // EFI 1.1+

	//
	// Open and Close Protocol Services
	//
	EFI_OPEN_PROTOCOL OpenProtocol; // EFI 1.1+
	EFI_CLOSE_PROTOCOL CloseProtocol; // EFI 1.1+
	EFI_OPEN_PROTOCOL_INFORMATION OpenProtocolInformation; // EFI 1.1+

	//
	// Library Services
	//
	EFI_PROTOCOLS_PER_HANDLE ProtocolsPerHandle; // EFI 1.1+
	EFI_LOCATE_HANDLE_BUFFER LocateHandleBuffer; // EFI 1.1+
	EFI_LOCATE_PROTOCOL LocateProtocol; // EFI 1.1+
	EFI_INSTALL_MULTIPLE_PROTOCOL_INTERFACES
	InstallMultipleProtocolInterfaces; // EFI 1.1+
	EFI_UNINSTALL_MULTIPLE_PROTOCOL_INTERFACES
	UninstallMultipleProtocolInterfaces; // EFI 1.1+

	//
	// 32-bit CRC Services
	//
	EFI_CALCULATE_CRC32 CalculateCrc32; // EFI 1.1+

	//
	// Miscellaneous Services
	//
	EFI_COPY_MEM CopyMem; // EFI 1.1+
	EFI_SET_MEM
	SetMem; // EFI 1.1+UEFI Specification, Version 2.8 EFI System Table
	EFI_CREATE_EVENT_EX CreateEventEx; // UEFI 2.0+
} EFI_BOOT_SERVICES;

#define EFI_RUNTIME_SERVICES_SIGNATURE 0x56524553544e5552
#define EFI_RUNTIME_SERVICES_REVISION EFI_SPECIFICATION_VERSION
typedef struct {
	EFI_TABLE_HEADER Hdr;

	//
	// Time Services
	//
	EFI_GET_TIME GetTime;
	EFI_SET_TIME SetTime;
	EFI_GET_WAKEUP_TIME GetWakeupTime;
	EFI_SET_WAKEUP_TIME SetWakeupTime;

	//
	// Virtual Memory Services
	//
	EFI_SET_VIRTUAL_ADDRESS_MAP SetVirtualAddressMap;
	EFI_CONVERT_POINTER ConvertPointer;

	//
	// Variable Services
	//
	EFI_GET_VARIABLE GetVariable;
	EFI_GET_NEXT_VARIABLE_NAME GetNextVariableName;
	EFI_SET_VARIABLE SetVariable;

	//
	// Miscellaneous Services
	//
	EFI_GET_NEXT_HIGH_MONOTONIC_COUNT GetNextHighMonotonicCount;
	EFI_RESET_SYSTEM ResetSystem;

	//
	// UEFI 2.0 Capsule Services
	//
	EFI_UPDATE_CAPSULE UpdateCapsule;
	EFI_QUERY_CAPSULE_CAPABILITIES QueryCapsuleCapabilities;

	//
	// Miscellaneous UEFI 2.0 Service
	//
	EFI_QUERY_VARIABLE_INFO QueryVariableInfo;
} EFI_RUNTIME_SERVICES;

typedef struct {
	EFI_GUID VendorGuid;
	VOID *VendorTable;
} EFI_CONFIGURATION_TABLE;

#define ACPI_TABLE_GUID \
	{ 0xeb9d2d30,       \
	  0x2d88,           \
	  0x11d3,           \
	  { 0x9a, 0x16, 0x00, 0x90, 0x27, 0x3f, 0xc1, 0x4d } }

#define SAL_SYSTEM_TABLE_GUID \
	{ 0xeb9d2d32,             \
	  0x2d88,                 \
	  0x11d3,                 \
	  { 0x9a, 0x16, 0x00, 0x90, 0x27, 0x3f, 0xc1, 0x4d } }

#define SMBIOS_TABLE_GUID \
	{ 0xeb9d2d31,         \
	  0x2d88,             \
	  0x11d3,             \
	  { 0x9a, 0x16, 0x00, 0x90, 0x27, 0x3f, 0xc1, 0x4d } }

#define SMBIOS3_TABLE_GUID \
	{ 0xf2fd1544,          \
	  0x9794,              \
	  0x4a2c,              \
	  { 0x99, 0x2e, 0xe5, 0xbb, 0xcf, 0x20, 0xe3, 0x94 } }

#define MPS_TABLE_GUID \
	{ 0xeb9d2d2f,      \
	  0x2d88,          \
	  0x11d3,          \
	  { 0x9a, 0x16, 0x00, 0x90, 0x27, 0x3f, 0xc1, 0x4d } }

//
// ACPI 2.0 or newer tables should use EFI_ACPI_TABLE_GUID
//
#define EFI_ACPI_TABLE_GUID \
	{ 0x8868e871,           \
	  0xe4f1,               \
	  0x11d3,               \
	  { 0xbc, 0x22, 0x00, 0x80, 0xc7, 0x3c, 0x88, 0x81 } }
#define EFI_ACPI_20_TABLE_GUID EFI_ACPI_TABLE_GUID

#define ACPI_TABLE_GUID \
	{ 0xeb9d2d30,       \
	  0x2d88,           \
	  0x11d3,           \
	  { 0x9a, 0x16, 0x00, 0x90, 0x27, 0x3f, 0xc1, 0x4d } }
#define ACPI_10_TABLE_GUID ACPI_TABLE_GUID

#define EFI_JSON_CONFIG_DATA_TABLE_GUID \
	{ 0x87367f87,                       \
	  0x1119,                           \
	  0x41ce,                           \
	  { 0xaa, 0xec, 0x8b, 0xe0, 0x11, 0x1f, 0x55, 0x8a } }

#define EFI_JSON_CAPSULE_DATA_TABLE_GUID \
	{ 0x35e7a725,                        \
	  0x8dd2,                            \
	  0x4cac,                            \
	  { 0x80, 0x11, 0x33, 0xcd, 0xa8, 0x10, 0x90, 0x56 } }

#define EFI_JSON_CAPSULE_RESULT_TABLE_GUID \
	{ 0xdbc461c3,                          \
	  0xb3de,                              \
	  0x422a,                              \
	  { 0xb9, 0xb4, 0x98, 0x86, 0xfd, 0x49, 0xa1, 0xe5 } }

#define EFI_PROPERTIES_TABLE_VERSION 0x00010000
typedef struct {
	UINT32 Version;
	UINT32 Length;
	UINT32 MemoryProtectionAttribute;
} EFI_PROPERTIES_TABLE;
//
// Memory attribute (Non-defined bits are reserved)
//
#define EFI_PROPERTIES_RUNTIME_MEMORY_PROTECTION_NON_EXECUTABLE_PE_DATA 0x1
// BIT 0 - description - implies the runtime data is separated from the code

#define EFI_ACPI_TABLE_PROTOCOL_GUID \
	{ 0xffe06bdd,                    \
	  0x6107,                        \
	  0x46a6,                        \
	  { 0x7b, 0xb2, 0x5a, 0x9c, 0x7e, 0xc5, 0x27, 0x5c } }

#define EFI_MEMORY_ATTRIBUTES_TABLE_GUID \
	{ 0xdcfa911d,                        \
	  0x26eb,                            \
	  0x469f,                            \
	  { 0xa2, 0x20, 0x38, 0xb7, 0xdc, 0x46, 0x12, 0x20 } }

//
// EFI_MEMORY_ATTRIBUTES_TABLE
//
typedef struct {
	UINT32 Version;
	UINT32 NumberOfEntries;
	UINT32 DescriptorSize;
	UINT32 Reserved;
	// EFI_MEMORY_DESCRIPTOR Entry[1];
} EFI_MEMORY_ATTRIBUTES_TABLE;

#define EFI_SYSTEM_TABLE_SIGNATURE 0x5453595320494249
#define EFI_2_80_SYSTEM_TABLE_REVISION ((2 << 16) | (80))
#define EFI_2_70_SYSTEM_TABLE_REVISION ((2 << 16) | (70))
#define EFI_2_60_SYSTEM_TABLE_REVISION ((2 << 16) | (60))
#define EFI_2_50_SYSTEM_TABLE_REVISION ((2 << 16) | (50))
#define EFI_2_40_SYSTEM_TABLE_REVISION ((2 << 16) | (40))
#define EFI_2_31_SYSTEM_TABLE_REVISION ((2 << 16) | (31))
#define EFI_2_30_SYSTEM_TABLE_REVISION ((2 << 16) | (30))
#define EFI_2_20_SYSTEM_TABLE_REVISION ((2 << 16) | (20))
#define EFI_2_10_SYSTEM_TABLE_REVISION ((2 << 16) | (10))
#define EFI_2_00_SYSTEM_TABLE_REVISION ((2 << 16) | (00))
#define EFI_1_10_SYSTEM_TABLE_REVISION ((1 << 16) | (10))
#define EFI_1_02_SYSTEM_TABLE_REVISION ((1 << 16) | (02))
typedef struct {
	EFI_TABLE_HEADER Hdr;
	CHAR16 *FirmwareVendor;
	UINT32 FirmwareRevision;
	EFI_HANDLE ConsoleInHandle;
	EFI_SIMPLE_TEXT_INPUT_PROTOCOL *ConIn;
	EFI_HANDLE ConsoleOutHandle;
	EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *ConOut;
	EFI_HANDLE StandardErrorHandle;
	EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *StdErr;
	EFI_RUNTIME_SERVICES *RuntimeServices;
	EFI_BOOT_SERVICES *BootServices;
	UINTN NumberOfTableEntries;
	EFI_CONFIGURATION_TABLE *ConfigurationTable;
} EFI_SYSTEM_TABLE;

#define EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID \
	{ 0x9042a9de,                         \
	  0x23dc,                             \
	  0x4a38,                             \
	  { 0x96, 0xfb, 0x7a, 0xde, 0xd0, 0x80, 0x51, 0x6a } }

struct EFI_GRAPHICS_OUTPUT_PROTOCOL;

typedef enum {
	PixelRedGreenBlueReserved8BitPerColor,
	PixelBlueGreenRedReserved8BitPerColor,
	PixelBitMask,
	PixelBltOnly,
	PixelFormatMax
} EFI_GRAPHICS_PIXEL_FORMAT;

typedef struct {
	UINT32 RedMask;
	UINT32 GreenMask;
	UINT32 BlueMask;
	UINT32 ReservedMask;
} EFI_PIXEL_BITMASK;

typedef struct {
	UINT32 Version;
	UINT32 HorizontalResolution;
	UINT32 VerticalResolution;
	EFI_GRAPHICS_PIXEL_FORMAT PixelFormat;
	EFI_PIXEL_BITMASK PixelInformation;
	UINT32 PixelsPerScanLine;
} EFI_GRAPHICS_OUTPUT_MODE_INFORMATION;

typedef EFI_STATUS(EFIAPI *EFI_GRAPHICS_OUTPUT_PROTOCOL_QUERY_MODE)(
	IN struct EFI_GRAPHICS_OUTPUT_PROTOCOL *This, IN UINT32 ModeNumber,
	OUT UINTN *SizeOfInfo, OUT EFI_GRAPHICS_OUTPUT_MODE_INFORMATION **Info);

typedef EFI_STATUS(EFIAPI *EFI_GRAPHICS_OUTPUT_PROTOCOL_SET_MODE)(
	IN struct EFI_GRAPHICS_OUTPUT_PROTOCOL *This, IN UINT32 ModeNumber);

typedef struct {
	UINT8 Blue;
	UINT8 Green;
	UINT8 Red;
	UINT8 Reserved;
} EFI_GRAPHICS_OUTPUT_BLT_PIXEL;
typedef enum {
	EfiBltVideoFill,
	EfiBltVideoToBltBuffer,
	EfiBltBufferToVideo,
	EfiBltVideoToVideo,
	EfiGraphicsOutputBltOperationMax
} EFI_GRAPHICS_OUTPUT_BLT_OPERATION;
typedef EFI_STATUS(EFIAPI *EFI_GRAPHICS_OUTPUT_PROTOCOL_BLT)(
	IN struct EFI_GRAPHICS_OUTPUT_PROTOCOL *This,
	IN OUT EFI_GRAPHICS_OUTPUT_BLT_PIXEL *BltBuffer,
	OPTIONAL IN EFI_GRAPHICS_OUTPUT_BLT_OPERATION BltOperation,
	IN UINTN SourceX, IN UINTN SourceY, IN UINTN DestinationX,
	IN UINTN DestinationY, IN UINTN Width, IN UINTN Height,
	IN UINTN Delta OPTIONAL);

typedef struct {
	UINT32 MaxMode;
	UINT32 Mode;
	EFI_GRAPHICS_OUTPUT_MODE_INFORMATION *Info;
	UINTN SizeOfInfo;
	EFI_PHYSICAL_ADDRESS FrameBufferBase;
	UINTN FrameBufferSize;
} EFI_GRAPHICS_OUTPUT_PROTOCOL_MODE;

typedef struct EFI_GRAPHICS_OUTPUT_PROTOCOL {
	EFI_GRAPHICS_OUTPUT_PROTOCOL_QUERY_MODE QueryMode;
	EFI_GRAPHICS_OUTPUT_PROTOCOL_SET_MODE SetMode;
	EFI_GRAPHICS_OUTPUT_PROTOCOL_BLT Blt;
	EFI_GRAPHICS_OUTPUT_PROTOCOL_MODE *Mode;
} EFI_GRAPHICS_OUTPUT_PROTOCOL;

//
// ImageEntryPoint
//
typedef EFI_STATUS(EFIAPI *EFI_IMAGE_ENTRY_POINT)(
	IN EFI_HANDLE ImageHandle, IN EFI_SYSTEM_TABLE *SystemTable);

#define EFI_MP_SERVICES_PROTOCOL_GUID \
	{ 0x3fdda605,                     \
	  0xa76e,                         \
	  0x4f46,                         \
	  { 0xad, 0x29, 0x12, 0xf4, 0x53, 0x1b, 0x3d, 0x08 } }

struct EFI_MP_SERVICES_PROTOCOL;

typedef EFI_STATUS(EFIAPI *EFI_MP_SERVICES_GET_NUMBER_OF_PROCESSORS)(
	IN struct EFI_MP_SERVICES_PROTOCOL *This, OUT UINTN *NumberOfProcessors,
	OUT UINTN *NumberOfEnabledProcessors);

typedef struct {
	UINT32 Package;
	UINT32 Core;
	UINT32 Thread;
} EFI_CPU_PHYSICAL_LOCATION;
typedef struct {
	UINT64 ProcessorId;
	UINT32 StatusFlag;
	EFI_CPU_PHYSICAL_LOCATION Location;
} EFI_PROCESSOR_INFORMATION;
#define PROCESSOR_AS_BSP_BIT 0x00000001
#define PROCESSOR_ENABLED_BIT 0x00000002
#define PROCESSOR_HEALTH_STATUS_BIT 0x00000004
typedef EFI_STATUS(EFIAPI *EFI_MP_SERVICES_GET_PROCESSOR_INFO)(
	IN struct EFI_MP_SERVICES_PROTOCOL *This, IN UINTN ProcessorNumber,
	OUT EFI_PROCESSOR_INFORMATION *ProcessorInfoBuffer);

#define END_OF_CPU_LIST 0xffffffff
typedef VOID(EFIAPI *EFI_AP_PROCEDURE)(IN VOID *ProcedureArgument);
typedef EFI_STATUS(EFIAPI *EFI_MP_SERVICES_STARTUP_ALL_APS)(
	IN struct EFI_MP_SERVICES_PROTOCOL *This, IN EFI_AP_PROCEDURE Procedure,
	IN BOOLEAN SingleThread, IN EFI_EVENT WaitEvent OPTIONAL,
	IN UINTN TimeoutInMicroSeconds, IN VOID *ProcedureArgument OPTIONAL,
	OUT UINTN **FailedCpuList OPTIONAL);

typedef EFI_STATUS(EFIAPI *EFI_MP_SERVICES_STARTUP_THIS_AP)(
	IN struct EFI_MP_SERVICES_PROTOCOL *This, IN EFI_AP_PROCEDURE Procedure,
	IN UINTN ProcessorNumber, IN EFI_EVENT WaitEvent OPTIONAL,
	IN UINTN TimeoutInMicroseconds, IN VOID *ProcedureArgument OPTIONAL,
	OUT BOOLEAN *Finished OPTIONAL);

typedef EFI_STATUS(EFIAPI *EFI_MP_SERVICES_SWITCH_BSP)(
	IN struct EFI_MP_SERVICES_PROTOCOL *This, IN UINTN ProcessorNumber,
	IN BOOLEAN EnableOldBSP);

typedef EFI_STATUS(EFIAPI *EFI_MP_SERVICES_ENABLEDISABLEAP)(
	IN struct EFI_MP_SERVICES_PROTOCOL *This, IN UINTN ProcessorNumber,
	IN BOOLEAN EnableAP, IN UINT32 *HealthFlag OPTIONAL);

typedef EFI_STATUS(EFIAPI *EFI_MP_SERVICES_WHOAMI)(
	IN struct EFI_MP_SERVICES_PROTOCOL *This, OUT UINTN *ProcessorNumber);

typedef struct EFI_MP_SERVICES_PROTOCOL {
	EFI_MP_SERVICES_GET_NUMBER_OF_PROCESSORS GetNumberOfProcessors;
	EFI_MP_SERVICES_GET_PROCESSOR_INFO GetProcessorInfo;
	EFI_MP_SERVICES_STARTUP_ALL_APS StartupAllAPs;
	EFI_MP_SERVICES_STARTUP_THIS_AP StartupThisAP;
	EFI_MP_SERVICES_SWITCH_BSP SwitchBSP;
	EFI_MP_SERVICES_ENABLEDISABLEAP EnableDisableAP;
	EFI_MP_SERVICES_WHOAMI WhoAmI;
} EFI_MP_SERVICES_PROTOCOL;

#define EFI_SIMPLE_POINTER_PROTOCOL_GUID \
	{ 0x31878c87,                        \
	  0xb75,                             \
	  0x11d5,                            \
	  { 0x9a, 0x4f, 0x00, 0x90, 0x27, 0x3f, 0xc1, 0x4d } }

typedef struct {
	UINT64 ResolutionX;
	UINT64 ResolutionY;
	UINT64 ResolutionZ;
	BOOLEAN LeftButton;
	BOOLEAN RightButton;
} EFI_SIMPLE_POINTER_MODE;

struct _EFI_SIMPLE_POINTER_PROTOCOL;

typedef struct {
	INT32 RelativeMovementX;
	INT32 RelativeMovementY;
	INT32 RelativeMovementZ;
	BOOLEAN LeftButton;
	BOOLEAN RightButton;
} EFI_SIMPLE_POINTER_STATE;

typedef EFI_STATUS(EFIAPI *EFI_SIMPLE_POINTER_RESET)(
	IN struct _EFI_SIMPLE_POINTER_PROTOCOL *This,
	IN BOOLEAN ExtendedVerification);

typedef EFI_STATUS(EFIAPI *EFI_SIMPLE_POINTER_GET_STATE)(
	IN struct _EFI_SIMPLE_POINTER_PROTOCOL *This,
	IN OUT EFI_SIMPLE_POINTER_STATE *State);

typedef struct _EFI_SIMPLE_POINTER_PROTOCOL {
	EFI_SIMPLE_POINTER_RESET Reset;
	EFI_SIMPLE_POINTER_GET_STATE GetState;
	EFI_EVENT WaitForInput;
	EFI_SIMPLE_POINTER_MODE *Mode;
} EFI_SIMPLE_POINTER_PROTOCOL;

// ACPI tables

typedef struct __attribute__((packed)) RSDP {
	UINT64 signature; // Should be "RSD PTR "
	UINT8 checksum; // Checksum for RSDP v1.0, all of v1.0 bytes added together should equal 0
	UINT8 OEMID[6]; // OEM supplied string
	UINT8 revision; // Revision of this structure
	UINT32 rsdtAddress; // Physical address of RSDT table

	// For revisions => 2
	UINT32 length; // Length of the table
	UINT64 xsdtAddress; // Physical address of XSDT table
	UINT8 extendedChecksum; // Checksum for the entire table, sum of all bytes should equal 0
	UINT8 reserves[3]; // Reserved field.
} RSDP;

typedef struct __attribute__((packed)) {
	uint32_t signature;
	uint32_t length;
	uint8_t revision;
	uint8_t checksum;
	uint8_t OEMID[6];
	uint8_t OEMTableID[8];
	uint32_t OEMRevision;
	uint32_t creatorID;
	uint32_t creatorRevision;
} SDTH; // System Description Table Header

typedef struct __attribute__((packed)) {
	SDTH header;
	uint32_t entry
		[]; // 32 bit physical memory addresses, number of entries is based on the length in the header
} RSDT;

typedef struct __attribute__((packed)) {
	SDTH header;
	uint64_t entry
		[]; // 64 bit physical memory addresses, number of entries is based on the length in the header
} XSDT;
