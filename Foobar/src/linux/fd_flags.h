#include <fcntl.h>

struct OpenFlags
{
    enum
    {
        CreateOnly = O_CREAT | O_EXCL,
        OpenOnly = 0,
        OpenOrCreate = O_CREAT,
        ReadOnly = O_RDONLY,
        ReadWrite = O_RDWR,
        WriteOnly = O_WRONLY,
    };
};

struct Permissions
{
    enum
    {
        None = 0,
        OwnerRead = S_IRUSR,
        OwnerWrite = S_IWUSR,
        OwnerReadWrite = OwnerRead | OwnerWrite,
        GroupRead = S_IRGRP,
        GroupWrite = S_IWGRP,
        GroupReadWrite = GroupRead | GroupWrite,
        OtherRead = S_IROTH,
        OtherWrite = S_IWOTH,
        OtherReadWrite = OtherRead | OtherWrite,
        AllReadWrite = OwnerReadWrite | GroupReadWrite | OtherReadWrite
    };
};
