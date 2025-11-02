# Présentation
## Introduction
`Look` est un langage de programmation compilé qui, à chaque variable, permet d'intégrer des données connues à la compilation, et modifiables, afin de faire des vérifications, ou assurer que des tests ont bien été effectués.
En ce sens, on peut voir ce langage comme un version compilée et plus poussée de `TypeScript`.

## Motivation
Supposons par exemple que l'on souhaite créer une fonction `gmean` qui renvoit $\sqrt{ab}$ si $a\ge0$ et $b\ge0$.
Une implémentation en C serait suivante :
```c
int gmean(int a, int b) {
	if (a >= 0 && b >= 0) {
		return sqrt(a*b); // from <math.h>
	} else {
		return -1;
	}
}

void test(void) {
	printf("%d\n", gmean(8,2)); // output: 4
	printf("%d\n", gmean(-9, 9)); // output: -1
}
```

Mais on peut voir deux problèmes à cette implémentation :
- on doit systématiquement vérifier si $a\ge0$ et $b\ge0$, alors que dans la plupart des appels, cela sera vrai (ou pire si on appelle plusieurs fois `gmean` avec un même nombre le test sera redondant)
- on ne voit les erreurs qu'au *runtime*, alors qu'elles auraient pu facilement être anticipées.


## Résolution
Pour cela, l'idée est de ne définir `gmean` à la compilation qu'à condition que `a` et `b` soient positifs. Ainsi, si `a` ou `b` est négatif, alors `gmean` ne sera tout simplement pas définie.

Pour cela, on crée une classe `Integer` *(qui dans le langage est standard et plus fournie)* qui au runtime ne contient qu'un `int` (la valeur), mais à la compilation contient une information sur le signe.


```look
class Integer {
	// compilation-time data
	for {
		sign: enum {UNKNOWN, ZERO, POSITIVE, NEGATIVE};

		@test
		isPositive{sign == $ZERO || $POSITIVE}
	}

	// runtime data
	value: i32;
}

function gmean(a: Integer[isPositive], b: Integer[isPositive]) {
	return std::sqrt(a.value * b.value);
}

function test() {
	const a1 = Integer{8}; // isPositive returns true
	const b1 = Integer{2};
	sys::println(gmean(a1, b1));

	const a2 = Integer{-9}; // isPositive returns false
	const b2 = Integer{9};
	sys::println(gmean(a2, b2)); // throws a compile-time error
}


```

> On verra plus tard comment les données dans `sign` sont définies.


On remarque même qu'en connaissant la valeur pendant la compilation, on pourrait directement appeler `gmean` à la compilation et remplacer l'appel directement par le résultat.
Voyons justement comment faire.



# Données 
## Surchage
Il est possible de surcharger des fonctions afin de faire des optimisations.
Voici un exemple, où l'on enregistre les données à la compilation, et où l'on peut tenter de calculer directement à la compilation.

Il est néanmoins à noter que les données peuvent être lues mais ne peuvent pas être modifiées avec cette méthode.

```look
class Integer {
	for {
		val: i32;
		val_known: bool;
		sign: enum {UNKNOWN, POSITIVE, NEGATIVE};

		@test
		isPositive{sign == $POSITIVE}
	}

	value: i32;
}

// default function
function gmean(a: Integer[isPositive], b: Integer[isPositive]) {
	return Integer<sign=$POSITIVE>{std::sqrt(a.value * b.value)};
}

// optimized function
class KnownPositiveInteger = Integer[val_known, isPositive];

@predictible // result known at compile-time
function geman(a: KnownPositiveInteger, b: KnownPositiveInteger) {
	const value = gmean(a::value, b::value);
	return Integer<sign=$POSITIVE, val=value>{value};
}

function test() {
	const a1 = Integer{8}; // type: Integer<sign=$POSITIVE, value=8, val_known=true>
	const b1 = Integer{2};
	const m1 = gmean(a1, b1); type: Integer<sign=$POSITIVE, value=4, val_known=true>

	const direct = m1.value; // compiled as "direct = 4;"
}


```

On interdit que des fonctions puissent modifier le type de leurs arguments. C'est aux classes de gérer leurs arguments.


## Méthodes
Une classe peut contenir des méthodes. Ces dernières sont déclarées dans la défintion de la classe, elle-même dans un fichier `.th`, et sont définies dans un fichier `.tc`, un peu comme en C.

Créeons par exemple une méthode qui met un `Integer` à sa valeur absolue :
```look
// Integer.th
class {
	for {
		val: i32;
		val_known: bool;
		sign: enum {UNKNOWN, POSITIVE, NEGATIVE};

		@test
		isPositive{sign == $POSITIVE}
	}

	value: i32;

	setToAbsolute();
}
```

```look
// Integer.tc
function setToAbsolute {
	if (this.value < 0) {
		this.value = -this.value;
	}
}
```


Mais cette implémentation ne modifie pas sign au compile time. On pourrait ainsi avoir :
```look
function test() {
	let a = Integer{-42};
	a.setToAbsolute(); // we should set sign from NEGATIVE to POSITIVE
	gmean(a, 10); // compile-time error because sign is NEGATIVE
}
```

On doit ainsi rajouter une fonction de contrôle pour spécifier qu'il y a des opérations à faire à la compilation.

On aura ainsi la classe suivante :
```look
class {
	for {
		val: i32;
		val_known: bool;
		sign: enum {UNKNOWN, POSITIVE, NEGATIVE};

		@test
		isPositive{sign == $POSITIVE}
	}

	value: i32;

	@controlled
	setToAbsolute();
}
```

On a ainsi ce code :
```look
// for compilation
@control
function setToAbsolute {
	this.sign = POSITIVE;
	if (this.val_known && this.val < 0) {
		this.val = -this.val;
	}
}

// for runtime
function setToAbsolute {
	if (this.value < 0) {
		this.value = -this.value;
	}
}
```


Ainsi, dans l'exemple `test`, l'erreur de compilation disparaîtra.


## Appels à des fonctions controllées





# Librairie standard



# Pointeurs



