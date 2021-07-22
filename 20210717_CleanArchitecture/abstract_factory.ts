class Pot { }
class Ingredient { }
class Soup { }
class Vegetable extends Ingredient { }
class Protein extends Ingredient { }

class HotPot {
  // HotPotクラスは↓のクラスに依存している。
  private pot: Pot;
  private soup: Soup;
  private protein: Protein;
  private vegetables: Vegetable[];
  private otherIngredients: Ingredient[];

  constructor(pot: Pot, soup: Soup, vegetables: Vegetable[], otherIngredients: Ingredient[], protein: Protein) {
    this.pot = pot;
    this.soup = soup;
    this.vegetables = vegetables;
    this.otherIngredients = otherIngredients;
    this.protein = protein;
  }

  public addSoup(soup: Soup) {
    this.soup = soup;
  }

  public addMain(protein: Protein) {
    this.protein = protein;
  }

  public addVegetables(vegetables: Vegetable[]) {
    this.vegetables = vegetables
  }

  public addOtherIngredients(otherIngredients: Ingredient[]) {
    this.otherIngredients = otherIngredients
  }
}

abstract class Factory {
  public abstract getSoup(): Soup;
  public abstract getMain(): Protein;
  public abstract getVegetables(): Vegetable[];
  public abstract getOtherIngredients(): Ingredient[];
}

class MizutakiFactory extends Factory {
  public getSoup() {
    return new Soup();
  }

  public getMain() {
    return new Protein();
  }

  public getVegetables() {
    const vegetables: Vegetable[] = [];
    vegetables.push(new Vegetable());
    vegetables.push(new Vegetable());
    vegetables.push(new Vegetable());

    return vegetables;
  }

  public getOtherIngredients() {
    const otherIngredients: Ingredient[] = [];
    otherIngredients.push(new Ingredient);
    otherIngredients.push(new Ingredient);
    return otherIngredients;
  }

  public hoge() {
    return 'hoge';
  }
}

type Type = 'mizutaki' | 'kimuchi' | 'katsuodashi';

class Application {
  public createHotPot(type: Type) {
    const factory = this.createFactory(type);
    const hotPot = new HotPot(
      new Pot(),
      factory.getSoup(),
      factory.getVegetables(),
      factory.getOtherIngredients(),
      factory.getMain()
    );
    return hotPot;
  }

  private createFactory(type: Type): Factory {
    if (type === 'mizutaki') {
      return new MizutakiFactory();
    }

    if (type === 'kimuchi') {
      return new MizutakiFactory();
    }

    if (type === 'katsuodashi') {
      return new MizutakiFactory();
    }

    return new MizutakiFactory();
  }
}


