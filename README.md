# Présentation
## Introduction
`Mirra` est un compilateur *a priori* ordinaire, à la différence près que chaque variable peut intégrer des données connues à la compilation pour faire des vérifications, ou assurer que des tests ont bien été effectués.
En ce sens, on peut voir ce langage comme un version compilée et plus poussée de `TypeScript`.

## Un premier exemple en C
Supposons que l'on travaille avec un `Player` qui puisse avoir *(ou non)* une `Weapon` à la main.

Une manière classique de représenter cela en C serait avec le code suivante :
```c
typedef struct {
	int type;

	// some large data

	union {
		// data according to the type of the weapon
	} data;
} Weapon;

typedef struct {
	Weapon* weapon;
} Player;

void Player_useWeapon(Player* player);


```

Mais le problème est le suivant : supposons qu'il faille rédiger `Player_useWeapon`
Pour cela, il faut s'assurer que l'arme puisse être utilisée par le joueur *(si dans notre jeu il y a des armes que notre joueur ne peut pas utiliser)*. Et de plus, il faut s'assurer que le pointeur sur weapon est valide.

Alors, plusieurs problèmes peuvent survenir :
- le développeur oublie qu'il faut effectuer ces vérifications, causant une *segfault* ou des *undefined behaviors*
- l'objet `Player` n'a pas été initalisé, resultant en une erreur
- si l'on inclue des vérifications sur la weapon au moment de `Player_useWeapon`, et qu'on l'utilise plusieurs fois, des instructions seront redondantes, augmentant le temps d'execution (bien sûr, ici cela est négligeable, mais sûr des sections plus critiques, mais on peut chercher à réduire le nombre d'instructions redondantes). On pourrait alors faire un test hors de `Player_useWeapon`, mais il y a là encore risque d'oubli.

## Solution
L'idée de `Mirra` est de charger le compilateur de faire toutes ces vérifications pour nous. Pour cela, chaque classe peut intégrer une *meta class*, qui possède des données stockées à la compilation mais inexistantes au moment de l'execution.
Ainsi, la *meta class* (que l'on rédigera) se chargera de faire toutes les vérifications pour nous.

## Implémentation
Voici une première implémentation naïve sans les outils qu'offre le langage.

```mirra
function isPlayerType(type: int): bool; // we don't care of the content

class MetaWeapon {
	typeKnown: bool = false;
	typeForPlayer: enum {UNKNOWN, TRUE, FALSE} = $UNKNOWN;
	type: int;


	setType(type: int) {
		this.typeKnown = true;
		this.type = type;

		if (isPlayerType(type)) {
			this.typeForPlayer = $TRUE;
		} else {
			this.typeForPlayer = $FALSE.
		}
	}

	markAsPlayerValid() {
		this.isForPlayer = $TRUE;
	}
}

@meta(MetaWeapon)
class Weapon {
	type: int;
	// add some other data

	isPlayerType() {
		// Optimization
		if (this::type == $TRUE) {
			return true;
		}

		if (this::type == $FALSE) {
			return false;
		}

		// Test at runtime
		return isPlayerType(type);
	}

	setType(type: int) {
		this.type = type;
		this::setType(type);
	}

	@require(isPlayerType)
	useAsPlayer(player: Player);
}






class MetaPlayer {
	isValidWeapon: enum {UNKNOWN, TRUE, FALSE} = UNKNOWN;
}

@meta(MetaPlayer)
class Player {
	weapon: Pointer<Weapon, $nullable, $readonly>;


	@require(this.weapon::isReadable == $TRUE)
	@require(this.weapon.get()::type == $TRUE)
	useWeapon() {
		/** no errors because:
		 * this.weapon is readable
		 * and this.weapon.get() satisfies useAsPlayer.
		 *
		 * required members have be set to true
		*/
		this.weapon.get().useAsPlayer(this);
	}

}


@main
function {
	let weapon: Weapon;
	weapon.setType(3); // it is a valid type for player
	compiler::println(weapon::typeForPlayer); // output: TRUE
	

	let player: Player;
	player.useWeapon(Pointer::to($weapon));

	// So we can call useWeapon
	player.useWeapon();
}
```


## Limites de l'implémentation

Notons qu'on aurait même pû exiger que le membre weapon soit valide directement en mettant, dans setWeapon, un @require(weapon::typeForPlayer == `$TRUE`).
De plus, l'utilisation des pointeurs n'est pas parfaite (notemment avec `get()`). On verra par la suite comment l'améliorer.

Enfin, on peut remarquer que si au runtime on modifie les données de weapon pour y mettre un type invalide, alors on peut executer useWeapon avec des données invalides.

Bien sûr, `Mirra` offre des outils pour lutter contre cela, que nous verrons par la suite.


# Structure










