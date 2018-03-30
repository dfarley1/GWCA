import os
import shutil

pub_dir = 'GWCA++-Public'
include_dirs = [
    'GWCA\\Constants',
    'GWCA\\Context',
    'GWCA\\GameContainers',
    'GWCA\\GameEntities',
    'GWCA\\Managers',
    'GWCA\\Packets',
    'GWCA\\Utilities'
    ]


os.system('make.bat')

for base_dir in include_dirs:
    for root, dirs, files in os.walk(base_dir):
        for file in files:
            dst = os.path.join(pub_dir, base_dir)
            if not os.path.exists(dst):
                     os.makedirs(dst)
            print('Copying', file, 'to', dst, '...')
            shutil.copy(os.path.join(root, file),os.path.join(dst, file))

shutil.copy('GWCA.lib',os.path.join(pub_dir, 'GWCA.lib'))
