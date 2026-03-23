# Custom Breakout Game with Editor

Proiect realizat în C++ folosind OpenGL (GFX Framework) și GLM pentru calcule matematice.

## Functionalitati

### 1. Editorul de Paleta
* Permite configurarea paletei prin plasarea de blocuri colorate pe o grila de 17x9.
* Suporta operatiuni de Drag & Drop (Click Dreapta) si stergere (Click Mijloc).
* Verifica automat regulile de constructie: paleta trebuie sa aiba 1-10 blocuri, sa fie pe un singur rand si fara spatii libere.
* Indicator vizual pentru numarul de blocuri ramase si validitatea designului.

### 2. Modul Gameplay (Breakout)
* Paleta din joc preia dimensiunea si culorile blocurilor alese in editor.
* Sistem de rezistenta (HP) pentru caramizi: Rosu (3 HP), Galben (2 HP), Verde/Gri (1 HP).
* Fizica pentru minge: coliziuni cerc-AABB si reflexii pe margini/paleta.
* Efecte vizuale: sistem de particule colorate si animatii de scalare la distrugerea caramizilor.
* Interfata: afisare scor si numar de vieti.

## Control
* Mouse Dreapta: Drag & Drop blocuri / Start joc.
* Mouse Mijloc: Stergere bloc din grila.
* Space: Lansare bila.
* Sageti Stanga/Dreapta: Miscare paleta.
