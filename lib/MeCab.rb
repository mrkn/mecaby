require 'mecaby'
require 'forwardable'

module MeCab
  class Tagger
    extend Forwardable

    def initialize(arg=nil)
      @tagger =
        case arg
        when Mecaby::Tagger
          arg
        when String
          Mecaby::Tagger.new(arg)
        when nil
          Mecaby::Tagger.new
        else
          raise ArgumentError, "invalid argument"
        end
    end

    def_delegator :@tagger, :parse

    def_delegator :@tagger, :nbest_parse, :parseNBest

    def_delegator :@tagger, :nbest_init, :parseNBestInit

    def_delegator :@tagger, :parse_to_node, :parseToNode
  end
end
