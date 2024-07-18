#
# Copyright (c) ZeroC, Inc. All rights reserved.
#

module Ice

    # The base class for all Slice classes.
    class Value
      def inspect
          ::Ice::__stringify(self, self.class::ICE_TYPE)
      end

      def ice_id()
          self.class::ICE_ID
      end

      def Value.ice_staticId()
          self::ICE_ID
      end

      def ice_getSlicedData()
          return _ice_slicedData
      end

      attr_accessor :_ice_slicedData  # Only used for instances of preserved classes.
    end

    T_Value = Ice.__declareClass('::Ice::Object')
    T_Value.defineClass(Value, -1, false, nil, [])

    class UnknownSlicedValue < Value
      def ice_id
          return @unknownTypeId
      end
    end

    T_UnknownSlicedValue = Ice.__declareClass('::Ice::UnknownSlicedValue')
    T_UnknownSlicedValue.defineClass(UnknownSlicedValue, -1, false, T_Value, [])

    class SlicedData
      attr_accessor :slices   # array of SliceInfo
    end

    class SliceInfo
      attr_accessor :typeId, :compactId, :bytes, :instances, :hasOptionalMembers, :isLastSlice
    end

    # The marshaling format for class instances.
    class FormatType
      include Comparable

      def initialize(val)
          fail("invalid value #{val} for FormatType") unless(val >= 0 and val < 3)
          @val = val
      end

      def FormatType.from_int(val)
          raise IndexError, "#{val} is out of range 0..2" if(val < 0 || val > 2)
          @@_values[val]
      end

      def to_s
          @@_names[@val]
      end

      def to_i
          @val
      end

      def <=>(other)
          other.is_a?(FormatType) or raise ArgumentError, "value must be a FormatType"
          @val <=> other.to_i
      end

      def hash
          @val.hash
      end

      def inspect
          @@_names[@val] + "(#{@val})"
      end

      def FormatType.each(&block)
          @@_values.each(&block)
      end

      @@_names = ['DefaultFormat', 'CompactFormat', 'SlicedFormat']
      @@_values = [FormatType.new(0), FormatType.new(1), FormatType.new(2)]

      DefaultFormat = @@_values[0]
      CompactFormat = @@_values[1]
      SlicedFormat = @@_values[2]

      private_class_method :new
    end
end
