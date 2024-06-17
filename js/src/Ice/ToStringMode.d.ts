declare module "ice" {
    namespace Ice {
        /**
         * The output mode for xxxToString method such as identityToString and proxyToString. The actual encoding format for
         * the string is the same for all modes: you don't need to specify an encoding format or mode when reading such a
         * string.
         */
        class ToStringMode {
            /**
             * Characters with ordinal values greater than 127 are kept as-is in the resulting string. Non-printable ASCII
             * characters with ordinal values 127 and below are encoded as \\t, \\n (etc.) or \\unnnn.
             */
            static readonly Unicode: ToStringMode;
            /**
             * Characters with ordinal values greater than 127 are encoded as universal character names in the resulting
             * string: \\unnnn for BMP characters and \\Unnnnnnnn for non-BMP characters. Non-printable ASCII characters
             * with ordinal values 127 and below are encoded as \\t, \\n (etc.) or \\unnnn.
             */
            static readonly ASCII: ToStringMode;
            /**
             * Characters with ordinal values greater than 127 are encoded as a sequence of UTF-8 bytes using octal escapes.
             * Characters with ordinal values 127 and below are encoded as \\t, \\n (etc.) or an octal escape. Use this mode
             * to generate strings compatible with Ice 3.6 and earlier.
             */
            static readonly Compat: ToStringMode;

            static valueOf(value: number): ToStringMode;
            equals(other: any): boolean;
            hashCode(): number;
            toString(): string;

            readonly name: string;
            readonly value: number;
        }
    }
}
