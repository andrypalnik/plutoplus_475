Щоб клонувати репозиторій разом із сабмодулями, використай цю команду:
git clone --recurse-submodules git@github.com:andrypalnik/plutoplus_475.git

Або, якщо вже клонували без сабмодулів:
git clone git@github.com:andrypalnik/plutoplus_475.git  
cd plutoplus_475  
git submodule update --init --recursive
