#include <windows.h>
#include <memory>

#pragma warning(disable: 26408) //warning C26408: Avoid malloc() and free(), prefer the nothrow version of new with delete (r.10).
#pragma warning(disable: 26446) //warning C26446: Prefer to use gsl::at() instead of unchecked subscript operator (bounds.4).
#pragma warning(disable: 26481) //warning C26481: Don't use pointer arithmetic. Use span instead (bounds.1).
#pragma warning(disable: 26482) //warning C26482: Only index into arrays using constant expressions (bounds.2).
#pragma warning(disable: 26485) //warning C26485: Expression 'fdi->FileName': No array to pointer decay (bounds.3).
#pragma warning(disable: 26490) //warning C26490: Don't use reinterpret_cast (type.1).

#pragma pack(push, 1)
typedef struct /*_Struct_size_bytes_( sizeof(FILE_DIRECTORY_INFORMATION) - sizeof(WCHAR) + FileNameLength )*/ _FILE_DIRECTORY_INFORMATION {
    /*_Field_range_( 0, ULONG_MAX ) */ ULONG FileNameLength;
    _Field_size_bytes_(FileNameLength) WCHAR FileName[1];
} FILE_DIRECTORY_INFORMATION, *PFILE_DIRECTORY_INFORMATION;
#pragma pack(pop)

constexpr rsize_t FDIFileNameRequiredBufferBytesWithNull( const rsize_t name_length ) {
	return ( ( name_length + 1 ) * sizeof( WCHAR ) );
	}

constexpr rsize_t FDIRequiredBufferBytes( const rsize_t name_length ) {
	//sizeof(FILE_DIRECTORY_INFORMATION) == 6U, includes two bytes for WCHAR[1]

	const rsize_t name_bytes = FDIFileNameRequiredBufferBytesWithNull( name_length );
	constexpr const rsize_t struct_size_no_filename = sizeof( FILE_DIRECTORY_INFORMATION ) - sizeof(WCHAR);
	const rsize_t struct_size_with_filename = struct_size_no_filename + name_bytes;
	return struct_size_with_filename;
	}


//Annotations do not seem to help.
//_At_( fdi->FileNameLength,  )
//_At_(fdi->FileName, _Pre_writable_byte_size_( fdi->FileNameLength ) )
//_Pre_satisfies_(_String_length_( src ) < ( fdi->FileNameLength ))
//_Pre_satisfies_( src_length < fdi->FileNameLength )
void copy_into_buffer( FILE_DIRECTORY_INFORMATION* const fdi, PCWSTR const src, const rsize_t src_length ) noexcept {
	//deliberately wrong destsz/_SizeInWords
	wcscpy_s(fdi->FileName, src_length, src);
	}

int main() {
	const wchar_t str_10[] = L"123456789";
	constexpr const rsize_t name_length = ( sizeof(str_10)/ sizeof(str_10[0]) );
	constexpr const rsize_t name_bytes = FDIFileNameRequiredBufferBytesWithNull( name_length );
	constexpr rsize_t struct_size_with_filename = FDIRequiredBufferBytes( name_length );
	void* const void_buffer_pointer = std::malloc( struct_size_with_filename );
	if( void_buffer_pointer == nullptr ) {
		return ERROR_OUTOFMEMORY;
		}


	FILE_DIRECTORY_INFORMATION* const fdi = static_cast<FILE_DIRECTORY_INFORMATION*>(void_buffer_pointer);
	fdi->FileNameLength = name_bytes;

	//It complains here, but it doesn't know the correct size of the array?
	// warning C26483: Value 100 is outside the bounds (0, 0) of variable 'fdi->FileName'. Only index into arrays using constant expressions that are within bounds of the array (bounds.2).
	fdi->FileName[100] = 0;


	const wchar_t str_10_backwards[] = L"987654321";

	//All of these should be fine
	wcscpy_s(fdi->FileName, name_length, str_10);
	wcscpy_s(fdi->FileName, name_length, str_10_backwards);
	copy_into_buffer(fdi, str_10, name_length);
	copy_into_buffer(fdi, str_10_backwards, name_length);


	const wchar_t str_19[] = L"123456789123456789";
	
	//This is explicitly wrong. It should complain. It doesn't.
	if( sizeof(str_19) > fdi->FileNameLength) {
		constexpr const rsize_t str_19_length = sizeof(str_19)/sizeof(wchar_t);
		wcscpy_s(fdi->FileName, str_19_length, str_19 );
		copy_into_buffer(fdi, str_19, str_19_length);
		}

	//Ok, it should complain about this, and it does.
	//Buffer overrun while writing to 'raw_buffer_pointer':  the writable size is '26' bytes, but '1001' bytes might be written.SAL_buffersize_test	
	{
		char* const raw_buffer_pointer = static_cast<char*>( void_buffer_pointer );
		raw_buffer_pointer[ 1000 ] = 0;
	}
	//and, it should complain about this...
	{
		//here, it does
		//warning C6386: Buffer overrun while writing to 'pointer_to_fdi':  the writable size is '26' bytes, but '1001' bytes might be written.
		char* const pointer_to_fdi = reinterpret_cast<char*>( fdi );
		pointer_to_fdi[ 1000 ] = 0;


		//here it doesn't
		FILE_DIRECTORY_INFORMATION* const bad_fdi = reinterpret_cast<FILE_DIRECTORY_INFORMATION*>( pointer_to_fdi + 1000 );
		bad_fdi->FileName[ 0 ] = 0;
		

		//It should know here that I'm past the length of the valid buffer?
		fdi->FileName[fdi->FileNameLength + 100] = 0;
	}

	std::free(void_buffer_pointer);

	}