# 📘 **README – Projet Tab5 Edge Computing : Détection & Alarme IR avec Dashboard Web**

## 📝 **Description du Projet**

Ce projet met en œuvre un **système autonome de détection de présence** basé sur une tablette **M5Stack Tab5**, intégrant :

* un capteur **IR filaire**
* un traitement **edge computing local**
* une interface tablette tactile (dashboard)
* un dashboard **Web en temps réel** via WebSocket
* un système d’alarme sonore activable/désactivable
* un historique local + graphique temps réel
* une authentification simple avant accès au site web *(optionnel)*

L’ensemble fonctionne **sans cloud**, uniquement en réseau local WiFi.

---

## 🔧 Matériel & Type de Capteur

### 🧱 Matériel

* 1 × **M5Stack Tab5** (ESP32-S3, écran tactile)
* 1 × **capteur IR digital de proximité** (type “obstacle avoidance”, sortie **DIGITALE**)
* Quelques fils Dupont / borniers selon le montage
* Réseau WiFi (box ou hotspot)

### 📟 Type de capteur IR

Le projet utilise un **capteur IR digital** :

* Sortie : **DIGITALE** (0 ou 1)
* Logique utilisée dans le code :

  * **LOW (0)** = présence détectée (**PROCHE**)
  * **HIGH (1)** = pas de détection (**LOIN**)
* Le capteur est lu avec `pinMode(IR_PIN, INPUT_PULLUP);` → la Tab5 active une résistance de pull-up interne.

> ➜ Concrètement : quand quelque chose passe devant le capteur, sa sortie est tirée à 0 (LOW)
> ce qui est interprété comme “PROCHE”.

---

## 🔌 Câblage et Ports utilisés

### 🧷 Broches Tab5 utilisées

Dans le code, on a :

```cpp
const int IR_PIN = 1;
```

Sur la Tab5, cela correspond à la **broche G1** (GPIO 1).

### 🧬 Connexions à réaliser

Capteur IR digital → Tab5 :

* **VCC du capteur** → **3.3V** de la Tab5
* **GND du capteur** → **GND** de la Tab5
* **OUT du capteur** → **G1** de la Tab5 (GPIO 1)

> ⚠ Important : alimenter le capteur en **3.3V**, pas en 5V, pour être sûr de rester dans les niveaux logiques compatibles.

Dans le code :

```cpp
const int IR_PIN = 1;          // GPIO 1 = G1 sur la Tab5
...
pinMode(IR_PIN, INPUT_PULLUP);
...
bool newRaw = (digitalRead(IR_PIN) == LOW);  // LOW = détection
```

---

# 📡 **Fonctionnalités Principales**

### 🔍 Détection IR

* Lecture du capteur IR filaire
* Filtrage logiciel de sensibilité
* États : **LOIN**, **PROCHE**, **ALERTE**
* Déclenchement d’une alarme après 2 secondes de présence continue

### 📊 Dashboard Tab5

* Affichage clair (PROCHE / LOIN / ALERTE)
* Graphique temps réel (60 points)
* Historique des 10 derniers événements
* Slider tactile pour régler la sensibilité
* Bouton tactile Reset
* Bouton tactile Son ON/OFF synchronisé avec le Web

### 🌐 Dashboard Web

* Radar animé en temps réel
* Bargraph minute/minute
* Indicateurs : état, détections, alarme
* Boutons : Reset, Son ON/OFF
* Connexion WebSocket pour synchronisation instantanée

### 🔔 Système d’alarme

* Bip sonore 1000 Hz
* Répétition tant que l’alarme est active
* Son désactivable via tablette ou navigateur

### 🔒 Authentification simple (optionnelle)

* Protection HTTP avec identifiant/mdp : `admin / admin`
* Facile à activer/désactiver dans le code

---

# 🧱 Architecture Technique

Le projet utilise :

| Élément            | Rôle                                            |
| ------------------ | ----------------------------------------------- |
| **Tab5**           | Traitement, affichage, serveur Web et WebSocket |
| **IR filaire**     | Détection de proximité                          |
| **WiFi**           | Accès au dashboard Web                          |
| **Navigateur Web** | Interface utilisateur distante                  |

---

# 🔐 Sécurité & Accès

### 1️⃣ Modifier l’identifiant & mot de passe du site Web

L’authentification se fait via **HTTP Basic**.
Dans ton code, ajoute ou modifie cette ligne dans le `handleRoot()` :

```cpp
void handleRoot() {
  if (!server.authenticate("admin", "admin")) {
      return server.requestAuthentication();
  }
  server.send(200, "text/html", webPage());
}
```

### Pour changer les identifiants :

```cpp
server.authenticate("TON_LOGIN", "TON_MDP");
```

Exemple :

```cpp
server.authenticate("admin", "admin");
```

---

# 🌐 Modifier le réseau WiFi dans le code

Dans le haut du fichier, tu as :

```cpp
const char* WIFI_SSID = "SKANDARPC7702";
const char* WIFI_PASS = "halouani";
```

Pour changer le réseau WiFi :

```cpp
const char* WIFI_SSID = "NOUVEAU_WIFI";
const char* WIFI_PASS = "NOUVEAU_MOT_DE_PASSE";
```

C’est tout.

---

# ⚙️ Installation

### 1️⃣ Pré-requis

* M5Stack Tab5 (ESP32-S3)
* IDE Arduino + M5Unified installé
* Capteur IR filaire actif LOW
* WiFi local

### 2️⃣ Flash du firmware

1. Brancher la Tab5
2. Sélectionner la carte :
   **M5Stack → M5Stack Tab5 (ESP32-S3)**
3. Compiler + téléverser

### 3️⃣ Accès au Dashboard Web

Quand la Tab5 est connectée, elle affiche son IP dans le moniteur série.

Dans un navigateur :

```
http://IP_DE_LA_TAB5/
```

---

# 🖥️ Utilisation

### Depuis la Tab5

* Appuyer sur "RESET" pour remettre le compteur à zéro
* Appuyer sur "SON ON/OFF" pour activer/désactiver l’alarme sonore
* Utiliser le slider pour changer la sensibilité du capteur IR

### Depuis le site Web

* Radar + bargraph en direct
* Boutons RESET et SON ON/OFF
* L’état est synchronisé instantanément avec la Tab5
---
Bien vu, tu as raison, il manque deux infos importantes pour ton prof :

* **quel capteur exactement**
* **quel port / broches utiliser sur la Tab5**

Je te redonne un **README complet** avec ces deux points bien expliqués (type de capteur + câblage / port).

---


# 🏆 Auteur

**Skandar – Étudiant SNPI 5 – Projet Edge Computing Tab5**

