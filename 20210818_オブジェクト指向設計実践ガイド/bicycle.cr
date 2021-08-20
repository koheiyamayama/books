class Gear
  getter :chainring, :cog, :wheel

  def initialize(@chainring : Int32, @cog : Int32, @wheel : Wheel? = nil)
  end

  def ratio
    chainring / cog.to_f
  end

  def gear_inches
    (wheel = self.wheel) ? ratio * diameter : 0
  end

  private def diameter
    wheel.diameter
  end
end

class Wheel
  getter :rim, :tire

  def initialize(@rim : Int32, @tire : Float64)
  end

  def diameter
    rim + (tire * 2)
  end

  def circumference
    diameter * Math::PI
  end
end

wheel = Wheel.new(rim: 26, tire: 1.5)
puts wheel.circumference

puts Gear.new(chainring: 52, cog: 11, wheel: wheel).gear_inches
puts Gear.new(chainring: 52, cog: 11).ratio
puts Gear.new(chainring: 52, cog: 11).gear_inches
