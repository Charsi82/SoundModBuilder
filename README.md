# SoundModBuilder
Сборщик мода голосовой озвучки для игры "Мир Кораблей" Lesta Games.

# Использование
1. устанавливаем Wwise2019.2.15.7667 (или распаковываем архив из папки utils в любую папку).
2. распаковываем проект для Wwise (wows_conversion_project19_Only_Windows.zip в папке utils в любую папку).
3. убеждаемся, что конвертер Wwise 2019.2.15.7667\Authoring\x64\Release\bin\WwiseCLI.exe запускается без ошибок.  
   В случаем возникновения ошибок устанавливаем распространяемый компонент Visual C++ https://aka.ms/vs/17/release/vc_redist.x64.exe.
4. настраиваем параметры в файле config.ini.

     -- путь к папке с игрой  
     GAME_PATH=E:\Program Files\Korabli

     -- путь к папке с wav файлами   
     SOURCE_DIRECTORY=E:\src_mod_wave

     -- название мода в меню настроек игры  
     MOD_CAPTION=MyMod

     -- имя папки мода с wem файлами в папке GAME_PATH\bin\<номер версии>\res_mods\banks\mods\  
     MOD_DIRECTORY=MyModDir

     -- префикс файлов озвучки  
     MOD_PREFIX=prefix  
     Если значение параметра MOD_PREFIX непустое, то файлы без префикса будут автоматически переименованы (prefix_Autopilot_Checkpoint.wav).  

     -- путь к wwiseCLI.exe версии 2019.2  
     WwiseCLIPath=...\Wwise 2019.2.15.7667\Authoring\x64\Release\bin\WwiseCLI.exe

     -- путь к проекту Wwise версии 2019.2  
     ProjectPath=...\wows_conversion_project19_Only_Windows\conv_19_WIN.wproj
   
5. все исходные файлы озвучки в формате wav собираем в одной папке SOURCE_DIRECTORY и даём им названия согласно описанию имён в файле utils\12.xslx.  
   Например, для события прохождения контрольной точки: Autopilot_Checkpoint.wav.  
   Для назначения нескольких файлов для одного события можно добавить к имени файла после символа подчёркивания порядковый номер файла. 
   Например, для события подтверждения нанесения критического повреждения: Hit_Confirmation_1.wave, Hit_Confirmation_2.wave, Hit_Confirmation_3.wave и т.д.
   Файлы с нестандартными именами не будут сконвертированы. 
   
6. запускаем SoundModBuilder.exe для конвертации wav файлов в формат wem и создания файла mod.xml в папке MOD_DIRECTORY.
