//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

namespace Ice
{
    [System.Serializable]
    public readonly struct Encoding : System.IEquatable<Encoding>
    {
        public readonly byte Major;
        public readonly byte Minor;

        public Encoding(byte major, byte minor)
        {
            Major = major;
            Minor = minor;
        }

        public override int GetHashCode() => System.HashCode.Combine(Major, Minor);

        public bool Equals(Encoding other) => Major.Equals(other.Major) && Minor.Equals(other.Minor);

        public override bool Equals(object? other)
        {
            if (ReferenceEquals(this, other))
            {
                return true;
            }
            return other is Encoding value && Equals(value);
        }

        public static bool operator ==(Encoding lhs, Encoding rhs) => Equals(lhs, rhs);
        public static bool operator !=(Encoding lhs, Encoding rhs) => !Equals(lhs, rhs);
    }
}
